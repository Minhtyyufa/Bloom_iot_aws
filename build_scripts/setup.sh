#!/bin/bash
# bucket name needs to be replaced everywhere


# Replace this with your bucket name created previously
BUCKET_NAME="bloom-kinesis-bucket-1"
find . -type f -name "*.json" -exec sed -i "s/bloom-kinesis-bucket-1/${BUCKET_NAME}/g" {} +

# CREATE DYNAMO DB TABLES
CO2_TABLE_ARN=$(aws dynamodb create-table --attribute-definitions AttributeName="readingID",AttributeType="S" --table-name "co2-level" --key-schema AttributeName="readingID",KeyType="HASH" --provisioned-throughput ReadCapacityUnits=5,WriteCapacityUnits=5 | jq .TableDescription.TableArn)

TEMP_TABLE_ARN=$(aws dynamodb create-table --attribute-definitions AttributeName="readingID",AttributeType="S" --table-name "air-temp-level" --key-schema AttributeName="readingID",KeyType="HASH" --provisioned-throughput ReadCapacityUnits=5,WriteCapacityUnits=5 | jq .TableDescription.TableArn)

LIQUID_TABLE_ARN=$(aws dynamodb create-table --attribute-definitions AttributeName="readingID",AttributeType="S" --table-name "liquid-level" --key-schema AttributeName="readingID",KeyType="HASH" --provisioned-throughput ReadCapacityUnits=5,WriteCapacityUnits=5 | jq .TableDescription.TableArn)

HUM_TABLE_ARN=$(aws dynamodb create-table --attribute-definitions AttributeName="readingID",AttributeType="S" --table-name "air-humidity-level" --key-schema AttributeName="readingID",KeyType="HASH" --provisioned-throughput ReadCapacityUnits=5,WriteCapacityUnits=5 | jq .TableDescription.TableArn)


# CREATE SOURCE STREAM FIREHOSE
aws iam create-role --role-name IoT-Source-Stream-Role --assume-role-policy-document file://source_stream_trust_policy.json

aws iam put-role-policy --role-name IoT-Source-Stream-Role --policy-name firehose_to_s3 --policy-document file://source_stream_role.json

sleep 10 

SOURCE_STREAM_ARN=$(aws firehose create-delivery-stream --delivery-stream-name IoT-Source-Stream --s3-destination-configuration file://source_stream.json | jq .DeliveryStreamARN)

echo "${SOURCE_STREAM_ARN}"

# CREATE IOT RULE AND PIPE IT TO SOURCE STREAM
aws iam create-role --role-name iot_to_firehose_role --assume-role-policy-document file://iot_rule_trust_policy.json

aws iam put-role-policy --role-name iot_to_firehose_role --policy-name iot_role_firehose --policy-document file://iot_rule_permissions.json

sleep 10 

aws iot create-topic-rule --rule-name IoT_to_Firehose --topic-rule-payload file://bloom_iot_rule.json

# CREATE DESTINATION DELIVERY STREAM 
DEST_DATA_STREAM_ROLE_NAME="IoT-Destination-Data-Stream-Role"

aws iam create-role --role-name ${DEST_DATA_STREAM_ROLE_NAME} --assume-role-policy-document file://source_stream_trust_policy.json

aws iam put-role-policy --role-name ${DEST_DATA_STREAM_ROLE_NAME} --policy-name destination_policy --policy-document file://destination_stream_permissions.json

sleep 10 

DEST_DATA_STREAM_ARN=$(aws firehose create-delivery-stream --delivery-stream-name IoT-Destination-Data-Stream --s3-destination-configuration file://destination_stream.json | jq .DeliveryStreamARN)

echo ${DEST_DATA_STREAM_ARN}

# CREATE AGGREGATE DELIVERY STREAM 
DEST_AGG_STREAM_ROLE_NAME="IoT-Destination-Aggregate-Stream-Role"

aws iam create-role --role-name ${DEST_AGG_STREAM_ROLE_NAME} --assume-role-policy-document file://source_stream_trust_policy.json

aws iam put-role-policy --role-name ${DEST_AGG_STREAM_ROLE_NAME} --policy-name destination_aggregate_policy --policy-document file://dest_agg_stream_permissions.json

sleep 10 

DEST_AGG_STREAM_ARN=$(aws firehose create-delivery-stream --delivery-stream-name IoT-Destination-Aggregate-Stream --s3-destination-configuration file://dest_agg_stream.json | jq .DeliveryStreamARN)

echo ${DEST_AGG_STREAM_ARN}

# CREATE KINESIS APPLICATION
KINESIS_APP_ROLE_NAME="Kinesis-Analytics-Bloom_IoT_Data-Role"

aws iam create-role --role-name ${KINESIS_APP_ROLE_NAME} --assume-role-policy-document file://kinesis_app_trust_policy.json

aws iam put-role-policy --role-name ${KINESIS_APP_ROLE_NAME} --policy-name kinesis_app_bloom_iot_data --policy-document file://kinesis_app_permissions.json

aws iam put-role-policy --role-name ${KINESIS_APP_ROLE_NAME} --policy-name kinesis_app_bloom_iot_data_alt --policy-document file://kinesis_app_permissions_alt.json

sleep 10 

aws kinesisanalytics create-application --application-name "Bloom_IoT_Data" --inputs file://kinesis_app_inputs.json --application-code file://kinesis_app_sql_statements

# CONNECT THE OTHER FIREHOSES AS THE DESTINATION
./connect_firehoses.sh

# CREATE THE LAMBDA FUNCTION

LAMBDA_FUNCTION_ROLE_NAME="lambda_dynamo_role"
LAMBDA_ROLE_ARN=$(aws iam create-role --role-name ${LAMBDA_FUNCTION_ROLE_NAME} --assume-role-policy-document file://lambda_trust_policy.json | jq .Role.Arn)

aws iam put-role-policy --role-name ${LAMBDA_FUNCTION_ROLE_NAME} --policy-name lambda_send_to_dynamo --policy-document file://lambda_permissions.json

temp="${LAMBDA_ROLE_ARN%\"}"
LAMBDA_ROLE_ARN="${temp#\"}"

echo "${LAMBDA_ROLE_ARN}"

sleep 10 

aws lambda create-function \
    --function-name send_to_dynamo \
    --runtime python3.6 \
    --zip-file fileb://lambda_functions/send_to_dynamo.zip \
    --handler send_to_dynamo.lambda_handler \
    --role ${LAMBDA_ROLE_ARN}

sleep 10 


APP_VERSION_ID=$(aws kinesisanalytics describe-application --application-name "Bloom_IoT_Data" | jq .ApplicationDetail.ApplicationVersionId)

aws kinesisanalytics add-application-output --application-name "Bloom_IoT_Data" --current-application-version-id ${APP_VERSION_ID} --application-output file://kinesis_app_add_lambda.json 

# START THE KINESIS APP
INPUT_STREAM_ID=$(aws kinesisanalytics describe-application --application-name "Bloom_IoT_Data" | jq .ApplicationDetail.InputDescriptions[0].InputId)

aws kinesisanalytics start-application --application-name "Bloom_IoT_Data" --input-configurations "Id=${INPUT_STREAM_ID},InputStartingPositionConfiguration={InputStartingPosition=NOW}"

# CREATE A COGNITO USER POOL
USER_POOL_ID=$(aws cognito-idp create-user-pool --pool-name bloom-pool | jq .UserPool.Id)
temp="${USER_POOL_ID%\"}"
USER_POOL_ID="${temp#\"}"

echo "User pool id: ${USER_POOL_ID}"

CLIENT_ID=$(aws cognito-idp create-user-pool-client --user-pool-id ${USER_POOL_ID} --client-name "bloom-app-client" --no-generate-secret | jq .UserPoolClient.ClientId)

echo "Your app client id: ${CLIENT_ID}"



IDENTITY_POOL_ID=$(aws cognito-identity create-identity-pool --identity-pool-name "bloom_identity_pool" --cognito-identity-providers "ProviderName=cognito-idp.us-east-1.amazonaws.com/${USER_POOL_ID},ClientId=${CLIENT_ID},ServerSideTokenCheck=TRUE" --no-allow-unauthenticated-identities | jq .IdentityPoolId)
temp="${IDENTITY_POOL_ID%\"}"
IDENTITY_POOL_ID="${temp#\"}"


sed "s/__IDENTITY_POOL_ID__/${IDENTITY_POOL_ID}/g" auth_identity_pool_trust_policy.json > temp_auth_identity_pool_trust_policy.json

sleep 1

AUTH_IDENTITY_POOL_ROLE_ARN=$(aws iam create-role --role-name "Bloom_auth_identity_pool_role" --assume-role-policy-document file://auth_identity_pool_trust_policy.json | jq .Role.Arn)

aws iam put-role-policy --role-name "Bloom_auth_identity_pool_role" --policy-name "auth_iot_permissions" --policy-document file://auth_identity_pool_permissions.json

aws cognito-identity set-identity-pool-roles --identity-pool-id ${IDENTITY_POOL_ID} --roles "authenticated=${AUTH_IDENTITY_POOL_ROLE_ARN}"
