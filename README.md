Emotech Test


We run an embedded version of Linux (Yocto) including concurrent services.
1) Write a simple client/server app that uses the gRPC communication protocol, to exchange data. Language C++. Data is one number, one string and a file (can be >1GB).
2) Write a script to test it. 


To run the server:
    ./file_transfer_server 


To run the client, specifying a string, a number and a file.  The file will be transferred to the server.  The file will have ".srv" appended to its filename so it can all sit in the same directory.

    ./file_transfer_client -s -ttext01 -n11 -ftest01.bin


To run the client, specifying a new string, a new number and a file.  The file will retrieved from the server.  The file will have ".from_srv" appended to its filename so it can sit in the same directory.

    ./file_transfer_client -r -ttext02 -n22 -ftest01.bin


The python script will test the file that is transferred.
