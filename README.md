# VisiRide - l2a-10-cloudcrackers
---
Smart 2FA system for electric scooter rentals


## Project structure

### Hardware Component
Located in `embedded/`
* `embedded/breakout/breakout/breakout_pcb/` contains breakout PCB for GPS, LTE, Camera (including Gerber files) developed in KiCAD V5 (https://www.kicad.org/download/)
* `embedded/` contains associated Quartus project files to build hardware system for DE1-SoC along with top level `on_board_computer.sv` to instantiate platform designer system on device
* `embedded/software/on_board_computer/dev/` contains firmware stack written from scratch for project
* `embedded/software/on_board_computer/FreeRTOS` contains FreeRTOS source code and port
* `embedded/software/on_board_computer/` contains makefiles, main, project files etc

DE1-SoC system was built using Quartus version 18.1 and embedded software compiled with included NIOS II build tools. 


### Cloud Server 
* The server is entirely contained in server.js
* facial_recognition library source: https://github.com/ageitgey/face_recognition

Written in Node.js 


### User Application 
Located in `dart/`
* `dart/lib/` is all written from scratch.
  * `dart/lib/api/` contains all the code required for facilitating api calls to the server using the http package.
  * `dart/lib/model/` contains custom dart objects for scooter and users.
  * `dart/lib/pages/` contains the different pages the user can interact with.
    * `dart/lib/pages/MapPage.dart` contains the map interface, provided by [Google Maps](https://developers.google.com/maps/), and contains the code to periodically update server with the user's current location grab the list of scooters.
    * `dart/lib/pages/Signup.dart` features an [image cropper provided by Yalantis](https://github.com/Yalantis/uCrop).
  * `dart/lib/utils/` contains classes to store some helper functions for the image picker and getting user data.
  * `dart/lib/widget/` contains custom flutter widgets
 * `dart/pubspec.yaml` contains all the imported [packages](https://pub.dev/) used, some are open-source, the others are written by the Flutter team.
 
 Apks were all built using Flutter in Android Studio.
 
 
### Cloud Database 
* Endpoints for modifying the database are defined in `server.js`
* MondoDB database is deployed on the cloud via an Azure VM

Written in Node.js 
