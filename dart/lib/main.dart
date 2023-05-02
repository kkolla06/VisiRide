import 'package:animated_theme_switcher/animated_theme_switcher.dart';
import 'package:dart/pages/LoginPage.dart';
import 'package:dart/pages/MapPage.dart';
import 'package:dart/pages/ProfilePage.dart';
import 'package:dart/pages/SignupPage.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:dart/themes.dart';
import 'package:dart/utils/userPreferences.dart';
import 'package:shared_preferences/shared_preferences.dart';


Future main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await SystemChrome.setPreferredOrientations([
    DeviceOrientation.portraitUp,
    DeviceOrientation.portraitDown,
  ]);
  final prefs = await SharedPreferences.getInstance();
  await prefs.clear();
  await UserPreferences.init();



  runApp(const ScooterApp());
}

class ScooterApp extends StatelessWidget {
  static const String title = 'User Profile';

  const ScooterApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ThemeProvider(
      initTheme: MyThemes.darkTheme,
      child: Builder(
        builder: (context) => MaterialApp(
          debugShowCheckedModeBanner: false,
          theme: ThemeData(colorScheme: const ColorScheme.dark()),
          title: title,
          home: const LoginPage(),
          routes: {
            '/login': (context) => const LoginPage(),
            '/signup': (context) => const SignupPage(),
            '/map':(context) => const MapPage(),
            '/profile':(context) => const ProfilePage(),
            '/signupPicture':(context)=>const SignupPicturePage(),
          },
        ),
      ),
    );
  }
}