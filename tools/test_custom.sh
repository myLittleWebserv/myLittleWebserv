#!/bin/bash

PORT1=7777
NEXT_TEST_MSG="Pleas press enter to next test."
GET_TEST="client/GET_TEST"
HEAD_TEST="client/HEAD_TEST"
POST_TEST="client/POST_TEST"
DELETE_TEST="client/DELETE_TEST"
UNKNOWN_TEST="client/UNKNOWN_TEST"
CGI_TEST="client/CGI_TEST"

# # POST TEST

echo "====================================== POST TEST ======================================"
read input

./myLittleClient $PORT1 $POST_TEST/PostRequest_0.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $POST_TEST/PostRequest_1.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $POST_TEST/PostRequest_2.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $POST_TEST/PostRequest_3.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $POST_TEST/PostRequest_4.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $POST_TEST/PostRequest_5.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $POST_TEST/PostRequest_6.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $POST_TEST/PostRequest_7.infile
echo $NEXT_TEST_MSG
read input


./myLittleClient $PORT1 $POST_TEST/PostRequest_8.infile
echo $NEXT_TEST_MSG
read input


./myLittleClient $PORT1 $POST_TEST/PostRequest_9.infile
echo $NEXT_TEST_MSG
read input


# GET TEST

echo "====================================== GET TEST ======================================"

./myLittleClient $PORT1 $GET_TEST/GetRequest_0.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $GET_TEST/GetRequest_1.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $GET_TEST/GetRequest_2.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $GET_TEST/GetRequest_3.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $GET_TEST/GetRequest_4.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $GET_TEST/GetRequest_5.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $GET_TEST/GetRequest_6.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $GET_TEST/GetRequest_7.infile
echo $NEXT_TEST_MSG
read input



# DELETE test

echo "====================================== DELETE TEST ======================================"

./myLittleClient $PORT1 $DELETE_TEST/DeleteRequest_0.infile
echo $NEXT_TEST_MSG
read input

./myLittleClient $PORT1 $DELETE_TEST/DeleteRequest_1.infile
echo $NEXT_TEST_MSG
read input


UNKNOWN test
./myLittleClient $PORT1 $UNKNOWN_TEST/UnknownRequest_0.infile
echo $NEXT_TEST_MSG
read input


# Cgi Test

echo "====================================== CGI TEST ======================================"

./myLittleClient $PORT1 $CGI_TEST/CgiRequest_0.infile
echo $NEXT_TEST_MSG
read input

