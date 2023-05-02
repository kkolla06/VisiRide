import 'package:animated_theme_switcher/animated_theme_switcher.dart';
import 'package:dart/widget/button_widget.dart';
import 'package:flutter/material.dart';
import 'package:maps_launcher/maps_launcher.dart';

import '../model/ScooterObj.dart';

class ScooterPage extends StatefulWidget{
  final Scooter scooter;
  const ScooterPage(this.scooter, {super.key});

  @override
  _ScooterPageState createState() => _ScooterPageState();
}

class _ScooterPageState extends State<ScooterPage>{

  @override
  Widget build(BuildContext context){
    return
      ThemeSwitchingArea(
          child: Builder(
              builder:(context) => Scaffold(
                appBar: AppBar(
                  leading: const BackButton(),
                ),
                body: ListView(
                  physics: const BouncingScrollPhysics(),
                  children: [
                    IconButton(
                      onPressed: (){},
                      icon: const Icon(
                          Icons.electric_scooter
                      ),
                      iconSize: 36,
                      color: Colors.lightGreen,
                    ),
                    const SizedBox(height: 24,),
                    ButtonWidget(text: 'Directions', onPressed: (){
                      MapsLauncher.launchCoordinates(widget.scooter.lat, widget.scooter.long);
                    }),

                  ],
                ),
              )
          )

      );
  }
}