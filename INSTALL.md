# Installing the software

Please see [BUILD.MD](./BUILD.MD) first

Start by editing [customize_scripts.sh](./customize_scripts.sh). Change the bucket name to the bucket name that you desire. Then change the ACCOUNT_ID variable to be your aws account number.

Run [customize_scripts.sh](./customize_scripts.sh) like so:

`
chmod 755 customize_scripts.sh
./customize_scripts.sh
`

If the bucket name already exists, change the bucket name and try again.

Once you've found the bucket name, go into the [setup.sh](./setup.sh) script and change the bucket name to match your bucket name. Then run the script like so:

`
chmod 755 setup.sh
./setup.sh
`
Take note of the printed values at the end.

## Setting up the Registration website
To set up the first create an empty GitHub repository and place the files located in the [bloom-iot-device-signup folder](./bloom-iot-device-signup) into the empty repository. Then go to the **AWS Amplify console** and click **Host web app**. Click GitHub, login to your GitHub account, and authorize AWS Amplify to access your GitHub account. Then select the repository containing the files in the [bloom-iot-device-signup folder](./bloom-iot-device-signup). Hit Continue until AWS Amplify starts deploying your web app. 


