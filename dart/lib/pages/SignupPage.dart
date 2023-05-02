import 'dart:convert';
import 'dart:io';
import 'package:geolocator/geolocator.dart';
import 'package:http/http.dart';
import 'package:image_cropper/image_cropper.dart';
import 'package:loading_animation_widget/loading_animation_widget.dart';
import 'package:path/path.dart' as path;
import 'package:animated_theme_switcher/animated_theme_switcher.dart';
import 'package:dart/pages/MapPage.dart';
import 'package:dart/utils/userPreferences.dart';
import 'package:dart/widget/textfield_widget.dart';
import 'package:flutter/material.dart';
import 'package:image_picker/image_picker.dart';
import 'package:path_provider/path_provider.dart';
import '../api/AuthAPI.dart';
import '../model/UserObj.dart';
import '../utils/ImageHelper.dart';

final imageHelper = ImageHelper();

class SignupPage extends StatefulWidget {
  const SignupPage({super.key});

  @override
  _SignupPageState createState() => _SignupPageState();
}
class _SignupPageState extends State<SignupPage> {

  String _username = '';
  String _password = '';
  bool invalidUsername = false, invalidPassword = false;
  User user = UserPreferences.getUser();

  @override
  Widget build(BuildContext context) {
    return ThemeSwitchingArea(
        child: Builder(
      builder: (context) => Scaffold(
        body: ListView(
          padding: const EdgeInsets.symmetric(horizontal: 32),
          physics: const BouncingScrollPhysics(),
          children: [
            const SizedBox(height: 128),
            TextFieldWidget(
              label: 'Username',
              text: _username,
              onChanged: (username) {
                _username = username;
                if(username.length >= 6 && !username.contains('.')){
                  setState(() {
                    invalidUsername = false;
                  });
                }
              },
            ),
            Opacity(
              opacity: invalidUsername ? 1 : 0,
              child: RichText(
                text: const TextSpan(
                  style: TextStyle(color: Color(0xffcf6679)),
                  text: 'Username must have at least 6 characters and cannot contain a period.',
                ),
              ),
            ),
            const SizedBox(height: 24),
            TextFieldWidget(
              label: 'Password',
              text: _password,
              obscureText: true,
              onChanged: (password) {
                _password = password;
                setState(() {
                });
              },
            ),

            const SizedBox(height: 48),
            ElevatedButton(
              style: ElevatedButton.styleFrom(
                shape: const StadiumBorder(),
                backgroundColor: (_username.length < 6 || _username.contains('.')) || _password.isEmpty ? const Color(0xff3700b3) : const Color(0xffbb86fc),
                foregroundColor: (_username.length < 6 || _username.contains('.')) || _password.isEmpty ? Colors.white70 : Colors.white,
                padding:
                    const EdgeInsets.symmetric(horizontal: 32, vertical: 12),
              ),
              child: const Text('Next'),
              onPressed: () {
                if(_username.length < 6 || _username.contains('.') ){
                  setState(() {
                    invalidUsername = true;
                  });
                }
                if(!invalidUsername && _password.isNotEmpty) {
                  user = user.copy(username: _username, password: _password);
                  UserPreferences.setUser(user);
                  Navigator.push(
                      context,
                      MaterialPageRoute(
                          builder: (context) => const SignupPicturePage()));
                }
              },
            )
          ],
        ),
      ),
    ));
  }
}

class SignupPicturePage extends StatefulWidget {
  const SignupPicturePage({super.key});

  @override
  State<StatefulWidget> createState() => _SignupPicturePageState();
}

class _SignupPicturePageState extends State<SignupPicturePage> {
  User user = UserPreferences.getUser();
  File? imageFile;
  final AuthAPI _authAPI = AuthAPI();

  bool isLoading = false;

  @override
  Widget build(BuildContext context) {
    return ThemeSwitchingArea(
        child: Builder(
            builder: (context) => Scaffold(
                  body: Stack(children:[
                    Opacity(opacity: isLoading ? 0.5: 1, child: Column(
                    children: [
                      const SizedBox(height: 150),
                      Center(
                          child: FittedBox(
                            fit: BoxFit.contain,
                            child: CircleAvatar(
                              backgroundColor: Colors.grey[300],
                              radius: 64,
                              foregroundImage: imageFile != null ? FileImage(imageFile!): null,
                              child: const Icon(
                                Icons.camera_alt_outlined,
                                size: 64,
                                color: Colors.grey,
                              ),
                            ),
                          ),
                      ),
                      const SizedBox(
                        height: 32,
                      ),
                      SizedBox(
                          height: 35,
                          width: 130,
                          child:
                      ElevatedButton(
                          onPressed: () async {
                        final file = await imageHelper.pickImage();
                        if(file != null){
                          final croppedFile = await imageHelper.crop(
                            file: file,
                            cropStyle: CropStyle.circle,
                          );
                          if(croppedFile != null){
                            setState(()=> imageFile = File(croppedFile.path));
                            print("IMAGE SIZE:------------------------------------------------------");
                            print(imageFile?.lengthSync());
                          }
                        }
                      },
                        style: ElevatedButton.styleFrom(
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(35),
                          ),
                          foregroundColor: Colors.white,
                          backgroundColor: const Color(0xffbb86fc),
                        ),
                          child: const Text('Choose Picture'),
                      )),
                      const SizedBox(
                        height: 16,
                      ),
                      SizedBox(
                        width: 130,
                        child:
                      ElevatedButton(
                        style: ElevatedButton.styleFrom(
                          shape: const StadiumBorder(),
                          backgroundColor: imageFile != null ? const Color(0xffbb86fc) : const Color(0xff3700b3),
                          foregroundColor: imageFile != null ? Colors.white : Colors.white30
                        ),
                        child: const Text('Sign Up'),
                        onPressed: () async {
                          if(imageFile != null){
                            user = user.copy(image64:base64Encode(imageFile!.readAsBytesSync()));
                          UserPreferences.setUser(user);
                          requestLocPerms();
                          Position currLoc = await Geolocator.getCurrentPosition(desiredAccuracy: LocationAccuracy.best);
                          await _authAPI.signUp(
                              user.username,
                              user.password,
                              imageFile!,
                              currLoc,
                          ).then((Response res){
                            if(res.statusCode == 200){
                              if(res.body == "Please provide a valid photo."){
                                _showPhotoError();
                              }
                              if(res.body == "New User Added!"){
                                Navigator.pushAndRemoveUntil(
                                    context,
                                    MaterialPageRoute(
                                        builder: (context) => MapPage()),
                                    ModalRoute.withName('/map'));
                              }
                              else{
                              }
                            }
                            else{
                              //Let's pray we don't get here
                              _show400Err();
                            }
                          });
                          }
                          },
                      )),
                    ],
                  )),
                    Opacity(opacity: isLoading ? 1 : 0 ,child: Center(child: LoadingAnimationWidget.threeArchedCircle(color: Colors.blue, size: 100))),
                  ])
                )));
  }
  Future<void> _showPhotoError() async{
    return showDialog<void>(
        context: context,
        barrierDismissible: true,
        builder: (BuildContext context) {
          return AlertDialog(
            title: const Text('Face not detected'),
            content: SingleChildScrollView(
              child: ListBody(
                children: const <Widget>[
                  Text('Please ensure your entire face is visible in the photo with good lighting.'),
                ],
              ),
            ),
            actions: <Widget>[
              TextButton(
                child: const Text('OK'),
                onPressed: () {
                  Navigator.of(context).pop();
                },
              ),
            ],
          );
        }
    );
  }

  Future<void> _show400Err() async {
    return showDialog<void>(
        context: context,
        barrierDismissible: false,
        builder: (BuildContext context) {
          return AlertDialog(
            title: const Text('Error'),
            content: SingleChildScrollView(
              child: ListBody(
                children: const <Widget>[
                  Text('Please try again later.'),
                ],
              ),
            ),
            actions: <Widget>[
              TextButton(
                child: const Text('OK'),
                onPressed: () {
                  Navigator.of(context).pop();
                },
              ),
            ],
          );
        }
    );
  }
  Future<void> _showLocPermsErr() async {
    return showDialog<void>(
        context: context,
        barrierDismissible: true,
        builder: (BuildContext context) {
          return AlertDialog(
            title: const Text('Error'),
            content: SingleChildScrollView(
              child: ListBody(
                children: const <Widget>[
                  Text('This app requires access to device location to be enabled to work. Please enable location permissions in the device settings.'),
                ],
              ),
            ),
            actions: <Widget>[
              TextButton(
                child: const Text('OK'),
                onPressed: () {
                  Navigator.of(context).pop();
                },
              ),
            ],
          );
        }
    );
  }

  Future<void> _showLocServErr() async {
    return showDialog<void>(
        context: context,
        barrierDismissible: true,
        builder: (BuildContext context) {
          return AlertDialog(
            title: const Text('Error'),
            content: SingleChildScrollView(
              child: ListBody(
                children: const <Widget>[
                  Text('This app requires device location services to be enabled to work. Please enable location services in the device settings.'),
                ],
              ),
            ),
            actions: <Widget>[
              TextButton(
                child: const Text('OK'),
                onPressed: () {
                  Navigator.of(context).pop();
                },
              ),
            ],
          );
        }
    );
  }

  void requestLocPerms() async{
    bool serviceEnabled;
    LocationPermission perms;

    serviceEnabled = await Geolocator.isLocationServiceEnabled();

    if(!serviceEnabled){
      _showLocServErr();
    }

    perms = await Geolocator.checkPermission();
    if(perms == LocationPermission.denied){
      perms = await Geolocator.requestPermission();
      if(perms == LocationPermission.denied){
        _showLocPermsErr();
        return Future.error('Location permissions are denied');
      }
    }
    if (perms == LocationPermission.deniedForever){
      _showLocPermsErr();
      return Future.error('Location permissions are permanently denied');
    }

  }
}
