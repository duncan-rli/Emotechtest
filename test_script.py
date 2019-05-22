#!/usr/bin/env python


import os
import filecmp
import subprocess

global_testFileName = "test02.bin"
global_1meg         = 1024 * 1024
global_testFileSize = 1024 #  multiples of 1m egabyte

def header():
    print "File_Transfer test script\n\n"


def createTestFile():
    f = open (global_testFileName, "wb+")

    barr = bytearray(global_1meg);

    # create 1M byte array
    for i in range (global_1meg):
        barr[i] = i & 0xff

    # create file multiples of 1 meg
    for i in range (global_testFileSize):
        f.write(barr)

    f.close()



def main():
    header()
    
    if os.path.isfile(global_testFileName):
        print "Re-using found test file\n"
    else:
        print "Creating large test file"
        createTestFile()

    # start server component
    server = subprocess.Popen("./file_transfer_server")

    print "Transfer file to server\n"
    print os.system("./file_transfer_client -s -ttext -n11 -f " + global_testFileName)

    print "Test: Compare transferred file\n"
    if filecmp.cmp(global_testFileName, global_testFileName+".srv"):
        print "File comparison passed\n"
    else:
        print "File comparison failed\n"

    print "Retrieve file from server\n"
    print os.system("./file_transfer_client -r -tstring -n22 -f " + global_testFileName)

    print "Test: Compare transferred file\n"
    if filecmp.cmp(global_testFileName, global_testFileName+".from_srv"):
        print "File comparison passed\n"
    else:
        print "File comparison failed\n"


    # kill the server object
    print "Killing server component"
    server.kill()



main()
