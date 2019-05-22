/*
 * EMOTECH Test Client side
 *
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <memory>

#include <vector>
#include "optionparser.h"

#include "convert_format.h"
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "emotechtest.grpc.pb.h"

#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using emotechtest::RequestFile;
using emotechtest::DataSet;
using emotechtest::FileDataChunk;
using emotechtest::NextChunkSeqId;
using emotechtest::StatusReply;
using emotechtest::FileTransfer;

class FileTransfer_Client {
 public:
    FileTransfer_Client(std::shared_ptr<Channel> channel)
      : stub_(FileTransfer::NewStub(channel)) {}

  std::string DataSendRequest(const int64_t& number, const std::string& text, const std::string& fileName) {
      // Data we are sending to the server.
      DataSet dataSet;
      dataSet.set_number(number);
      dataSet.set_text(text);
      dataSet.set_filename(fileName);

      // Container for the data we expect from the server.
      StatusReply reply;

      // Context for the client. It could be used to convey extra information to
      // the server and/or tweak certain RPC behaviors.
      ClientContext context;

      // The actual RPC.
      Status status = stub_->DataSendRequest(&context, dataSet, &reply);

      // Act upon its status.
      if (status.ok()) {
          return reply.replystatus();
      } else {
          std::cout << status.error_code() << ": " << status.error_message()
                    << std::endl;
          return "RPC failed";
      }
  }

  std::string SendFileChunk(const int64_t seq, const int64_t& chunkSize, std::string& pChunk, int64_t *pNextSeqId) {
      //
      FileDataChunk dataChunk;
      dataChunk.set_seq(seq);
      dataChunk.set_chunksize(chunkSize);
      dataChunk.set_chunk(pChunk);

      NextChunkSeqId reply;

      // Context for the client. It could be used to convey extra information to
      // the server and/or tweak certain RPC behaviors.
      ClientContext context;

      // The actual RPC.
      Status status = stub_->SendFileChunk(&context, dataChunk, &reply);

      // Act upon its status.
      if (status.ok()) {
          *pNextSeqId = reply.seqid();
          return std::to_string(reply.seqid());
      } else {
          std::cout << status.error_code() << ": " << status.error_message()
                    << std::endl;
          return "RPC failed";
      }
  }

  std::string DataRetrieveRequest(const std::string fileName, DataSet *ds) {
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    RequestFile reqFile;
    reqFile.set_filename(fileName);

    // The actual RPC.
    Status status = stub_->DataRetrieveRequest(&context, reqFile, ds);

    // Act upon its status.
    if (status.ok()) {
        return "Ok";
    } else {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
    }
  }

  std::string RequestFileChunk(int seqId, FileDataChunk& fdc) {
      // Context for the client. It could be used to convey extra information to
      // the server and/or tweak certain RPC behaviors.
      ClientContext context;
      NextChunkSeqId ncSeqId;
      ncSeqId.set_seqid(seqId);

      // The actual RPC.
      Status status = stub_->RequestFileChunk(&context, ncSeqId, &fdc);

      // Act upon its status.
      if (status.ok()) {
 //         *pNextSeqId = reply.seqid();
          return std::to_string(seqId);
      } else {
          std::cout << status.error_code() << ": " << status.error_message()
                    << std::endl;
          return "RPC failed";
      }
  }

  const size_t bufferSize = 1024 * 1024;

private:
  std::unique_ptr<FileTransfer::Stub> stub_;
};


void sendToServer(const int64_t num, const std::string& str, const std::string fileName){

    FileTransfer_Client fileTransfer(grpc::CreateChannel(
                                    "localhost:50051",
                                    grpc::InsecureChannelCredentials()));

    int64_t fileSize = 0;

    std::string reply = fileTransfer.DataSendRequest(num, str, fileName);
    std::cout << "fileTransfer received: " << reply << std::endl;

    std::ifstream inputFile(fileName);
    if (inputFile) {
        // get length of file:
        inputFile.seekg(0, inputFile.end);
        fileSize = inputFile.tellg();
        inputFile.seekg(0, inputFile.beg);
    }

    int64_t bufferSize = fileTransfer.bufferSize;

    // transfer buffer
    if (fileSize < bufferSize) {
        bufferSize = fileSize;
    }

    std::unique_ptr<char[]> buffer(new char[bufferSize]);
    int64_t seq = 0;
    int64_t remainingBytesToRead = fileSize;
    int64_t chunkSize = bufferSize;
    while (inputFile) {
        inputFile.read(buffer.get(), bufferSize);

        std::string chunk = Conv::toString(buffer.get(), bufferSize);

        // process data in buffer
        reply = fileTransfer.SendFileChunk(seq, chunkSize, chunk, &seq);
 //       std::cout << "fileTransfer received: " << reply << std::endl;

        remainingBytesToRead -= bufferSize;
        if (remainingBytesToRead < chunkSize)
            chunkSize = remainingBytesToRead;
        if (chunkSize < 0)
            chunkSize = 0;

    }

    // send last message to close file
    std::string emptyStr("");
    reply = fileTransfer.SendFileChunk(seq, 0, emptyStr, &seq);
    std::cout << "fileTransfer received done: " << reply << std::endl;

}


void getFromServer(const int64_t num, const std::string& str, const std::string& fileName) {
    FileTransfer_Client fileTransfer(grpc::CreateChannel(
                                    "localhost:50051",
                                    grpc::InsecureChannelCredentials()));

    DataSet ds;

    std::string reply = fileTransfer.DataRetrieveRequest(fileName, &ds);
    std::cout << "fileTransfer received: " << reply << std::endl;
    std::cout << "fileTransfer Server data: " << ds.number() << " " << ds.text() << " " << ds.filename()  << std::endl;

    int64_t seq = 0;
    std::ofstream outputFile(fileName +".from_srv");
    FileDataChunk fdc;
    fdc.set_chunksize(1);

    while (fdc.chunksize() > 0) {
        reply = fileTransfer.RequestFileChunk(seq, fdc);
        {
            // transfer buffer
            std::unique_ptr<char[]> buffer(new char[fileTransfer.bufferSize]);

            // convert chunk string to binary data
            bool b = Conv::fromString(fdc.chunk(), buffer.get());
            if (b) {
                outputFile.write(buffer.get(), (int64_t) fdc.chunksize());
            }

//            std::cout << "seq: " << fdc.seq() << " sz: " << fdc.chunk().length() << " " << fdc.chunksize()
//                      << std::endl;

            if (fdc.chunksize() == 0) {
                // close file
                outputFile.close();
            }
            seq = fdc.seq() + 1;
        }
    }

    std::cout << "fileTransfer written file: " << fileName+".from_srv" << std::endl;

}




int main(int argc, char** argv) {
    argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
    option::Stats stats(usage, argc, argv);

    // Parse parameter options
    option::Option options[stats.options_max], buffer[stats.buffer_max];
    option::Parser parse(usage, argc, argv, options, buffer);
    if (parse.error())
        return 1;

    if (options[HELP] || argc == 0)
    {
        printUsage();
        return 0;
    }


    char direction{' '};
    std::string text;
    std::string fileName;
    int64_t number = 0;

    // loop through command line parameters to set variables
    for (int i = 0; i < parse.optionsCount(); ++i)
    {
        option::Option& opt = buffer[i];
        switch (opt.index())
        {
            case HELP:
                // not possible, because handled further above and exits the program
            case OPTIONAL:
                break;
            case DIRECTION:
                direction = opt.name[0];
                break;
            case REQUIRED:
                break;
            case NUMERIC:
                if (opt.name[0] == 'n') {
                    number = std::strtod(opt.arg, nullptr);
                }
                break;
            case NONEMPTY:
                if (opt.name[0] == 't') {
                    text = std::string(opt.arg);
                }
                if (opt.name[0] == 'f'){
                    fileName = std::string(opt.arg);
                }
                break;
            case UNKNOWN:
                // not possible because Arg::Unknown returns ARG_ILLEGAL
                // which aborts the parse with an error
                break;
        }
    }


    switch (direction)
    {
        case 'r':
            getFromServer(number, text, fileName);
            break;
        case 's':
            sendToServer(number, text, fileName);
            break;
        default:
            printUsage();
            return 0;
    }

  return 0;
}

