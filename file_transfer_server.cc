/*
 * EMOTECH Test server side
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <fstream>

#include "convert_format.h"

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "emotechtest.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using emotechtest::RequestFile;
using emotechtest::DataSet;
using emotechtest::FileDataChunk;
using emotechtest::NextChunkSeqId;
using emotechtest::StatusReply;
using emotechtest::FileTransfer;


// Logic and data behind the server's behavior.
class FileTransfer_ServiceImpl final : public FileTransfer::Service {

  Status DataSendRequest (ServerContext *context, const DataSet *ds,
                StatusReply *reply) override {
      dataSet = *ds;
      reply->set_replystatus("OK");
      std::cout << "DataSet changed: Number: " << dataSet.number() << " Text: " << dataSet.text()
                << " Filename: " << dataSet.filename() << std::endl;

      return Status::OK;
  }

  Status SendFileChunk (ServerContext *context, const FileDataChunk *fdc,
                NextChunkSeqId *replySeqId) override {
      Status status = Status::OK;

      if (fdc->seq() == 0) {
          // create file
          outFile.open(dataSet.filename()+".srv", std::ios::binary);
      }

      // transfer buffer
      std::unique_ptr<char[]> buffer(new char[bufferSize]);

      // convert chunk string to binary data
      bool b = Conv::fromString(fdc->chunk(), buffer.get());
      if (b) {
          outFile.write(buffer.get(), (int64_t) fdc->chunksize());
      } else {
          status = Status::CANCELLED;
      }

//      std::cout << "srv seq: " << fdc->seq() << " sz: " << fdc->chunk().length() << " " << fdc->chunksize()
//            << "  b: " << b << std::endl;

      if (fdc->chunksize() == 0) {
          // close file
          outFile.close();
      }
      replySeqId->set_seqid(fdc->seq()+1);
      return status;
  }

  // retrieve data from server
  Status DataRetrieveRequest (ServerContext *context, const RequestFile *reqFile, DataSet* pds) override {
    dataSet.set_filename(reqFile->filename());

    pds->set_filename(dataSet.filename());
    pds->set_text(dataSet.text());
    pds->set_number(dataSet.number());

    buffSize = bufferSize;

    // create file object
    inFile.open(dataSet.filename()+".srv", std::ios::binary);
    if (inFile) {
          // get length of file:
          inFile.seekg(0, inFile.end);
          fileSize = inFile.tellg();
          inFile.seekg(0, inFile.beg);

          remainingBytesToRead = fileSize;
    }
    if (fileSize < bufferSize) {
          buffSize = fileSize;
    }

    chunkSize = buffSize;

    return Status::OK;
  }

  // Return next xhunk of file to requesting client
  Status RequestFileChunk(ServerContext *context, const NextChunkSeqId *seq, FileDataChunk *pFdc) override {
      FileDataChunk fdc;

      // transfer buffer
      std::unique_ptr<char[]> buffer(new char[buffSize]);

      inFile.read(buffer.get(), buffSize);

      size_t rd = inFile.gcount();
      if (chunkSize == 0) {
          // close file
          inFile.close();
      }

      // convert binary data to string
      std::string chunkString = Conv::toString(buffer.get(), buffSize);

//      std::cout << "srv seq: " << seq->seqid() << " sz: " << chunkString.length() << " rd: " << rd
//                << std::endl;

      if (chunkString.length() > 0) {
          pFdc->set_chunk(chunkString);
          pFdc->set_chunksize(chunkSize);
          pFdc->set_seq(seq->seqid());
      }

       remainingBytesToRead -= bufferSize;
       if (remainingBytesToRead < chunkSize)
           chunkSize = remainingBytesToRead;
       if (chunkSize < 0)
           chunkSize = 0;

       return Status::OK;
  }

  const size_t bufferSize = 1024 * 1024;
  size_t buffSize = bufferSize;
  int64_t remainingBytesToRead = 0;
  int64_t chunkSize = bufferSize;
  DataSet dataSet;

  std::string text;
  int64_t number = 0;

  // current input file object
  std::ifstream inFile;
  int64_t fileSize = 0;
  // current output file object
  std::ofstream outFile;

};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  FileTransfer_ServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
