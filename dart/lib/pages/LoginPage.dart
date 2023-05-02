import 'package:dart/pages/SignupPage.dart';
import 'package:dart/utils/userPreferences.dart';
import 'package:flutter/material.dart';
import 'package:dart/api/AuthAPI.dart';
import 'package:http/http.dart';
import 'package:loading_animation_widget/loading_animation_widget.dart';

import '../model/UserObj.dart';
import 'MapPage.dart';

class LoginPage extends StatefulWidget {
  const LoginPage({super.key});

  @override
  _LoginPageState createState() => _LoginPageState();
}

class _LoginPageState extends State<LoginPage> {
  String _username = '';
  String _password = '';
  late User user;
  final AuthAPI _authAPI = AuthAPI();
  bool invalidCreds = true;

  bool isLoading = false;

  @override
  void initState() {
    super.initState();
    user = UserPreferences.getUser();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        body: Stack(
        children: [
          Opacity(opacity: isLoading ? 0.5: 1, child:Center(
            child: Container(
      padding: const EdgeInsets.all(25.0),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const SizedBox(height: 100),
          TextField(
              decoration: const InputDecoration(hintText: 'Username'),
              onChanged: (value) {
                _username = value;
                setState(() {
                  invalidCreds = !(_username.isNotEmpty && _password.isNotEmpty);
                });
              }),
          const SizedBox(height: 15.0),
          TextField(
            decoration: const InputDecoration(hintText: 'Password'),
            onChanged: (value) {
              _password = value;
              setState(() {
                invalidCreds = !(_username.isNotEmpty && _password.isNotEmpty);
              });
            },
            obscureText: true,
          ),
          const SizedBox(
            height: 20.0,
          ),
          ElevatedButton(
            onPressed: () async {
              if(!invalidCreds) {
                setState(() {
                  isLoading = true;
                });
                await _authAPI.login(_username, _password).then((Response req) {
                  if (req.statusCode == 200) {
                    if (req.body != 'Incorrect username or password') {
                      user = user.copy(image64: req.body,
                          username: _username,
                          password: _password);
                      UserPreferences.setUser(user);
                      setState(() {
                        isLoading = false;
                      });
                      Navigator.pushAndRemoveUntil(
                          context,
                          MaterialPageRoute(builder: (context) => MapPage()),
                          ModalRoute.withName('/map'));
                    }
                    else {
                      _showIncorrectCreds();
                      setState(() {
                        isLoading = false;
                      });
                    }
                  }
                });
              }
            },
            style: ElevatedButton.styleFrom(
                foregroundColor: Colors.white,
                backgroundColor: invalidCreds ? const Color(0xff3700b3):const Color(0xffbb86fc),
                shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(20))),
            child: const Text('Login'),
          ),
          const SizedBox(height: 15),
          const Text('Don\'t have an account?'),
          ElevatedButton(
            onPressed: () {
              Navigator.push(context,
                  MaterialPageRoute(builder: (context) => const SignupPage()));
            },
            style: ElevatedButton.styleFrom(
                backgroundColor: const Color(0xffbb86fc),
                foregroundColor: Colors.white,
                shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(20))),
            child: const Text('Sign Up'),
          )
        ],
      ),
    ))),
          Opacity(opacity: isLoading? 1 : 0, child: Center(child: LoadingAnimationWidget.threeArchedCircle(color: Colors.blueAccent, size: 100))),
        ]
        ),
            
    );
  }

  Future<void> _showIncorrectCreds() async {
    return showDialog<void>(
        context: context,
        barrierDismissible: false,
        builder: (BuildContext context) {
          return AlertDialog(
            title: const Text('Error'),
            content: SingleChildScrollView(
              child: ListBody(
                children: const <Widget>[
                  Text('Incorrect email or password.'),
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
}
