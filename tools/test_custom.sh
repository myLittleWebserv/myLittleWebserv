#!/bin/bash

PORT1=7771
NEXT_TEST_MSG="Pleas press enter to next test."
GET_TEST="client/GET_TEST"
POST_TEST="client/POST_TEST"

# GET TEST

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


# POST TEST

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