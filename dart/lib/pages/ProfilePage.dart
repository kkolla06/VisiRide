import 'package:animated_theme_switcher/animated_theme_switcher.dart';
import 'package:dart/pages/LoginPage.dart';
import 'package:flutter/material.dart';
import 'package:dart/model/UserObj.dart';
import 'package:dart/utils/userPreferences.dart';
import 'package:dart/widget/profile_widget.dart';
import 'package:dart/widget/button_widget.dart';
import 'package:shared_preferences/shared_preferences.dart';

class ProfilePage extends StatefulWidget {
  const ProfilePage({super.key});

  @override
  _ProfilePageState createState() => _ProfilePageState();
}

class _ProfilePageState extends State<ProfilePage> {
  @override
  Widget build(BuildContext context) {
    User user = UserPreferences.getUser();

    return ThemeSwitchingArea(
      child: Builder(
        builder: (context) => Scaffold(
          appBar: AppBar(
            leading: const BackButton(),
          ),
          body: Column(
            children: [
              Center(
                child: ProfileWidget(
                  image64: user.image64,
                ),
              ),
              const SizedBox(height: 24),
              Text(
                user.username,
                style: const TextStyle(color: Colors.grey),
              ),
              ButtonWidget(
                  text: 'Log Out',
                  onPressed: () async {
                    SharedPreferences prefs =
                        await SharedPreferences.getInstance();
                    prefs.clear();
                    Navigator.pushAndRemoveUntil(
                        context,
                        MaterialPageRoute(
                            builder: (context) => const LoginPage()),
                        ModalRoute.withName('/login'));
                  }),
            ],
          ),
        ),
      ),
    );
  }
}
