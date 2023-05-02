import 'dart:async';
import 'dart:convert';

import 'package:dart/pages/ProfilePage.dart';
import 'package:dart/utils/userPreferences.dart';
import 'package:flutter/material.dart';
import 'package:geolocator/geolocator.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:loading_animation_widget/loading_animation_widget.dart';
import '../api/AuthAPI.dart';
import '../api/ScooterAPI.dart';
import '../model/ScooterObj.dart';
import 'ScooterPage.dart';
import 'package:http/http.dart' as http;
import 'package:latlong2/latlong.dart' as lat_lng;
import 'package:intl/intl.dart';

class MapPage extends StatefulWidget {
  const MapPage({super.key});

  @override
  _MapPageState createState() => _MapPageState();
}

class _MapPageState extends State<MapPage> {
  Completer<GoogleMapController> mapController = Completer();
  Set<Marker> _markers = {};
  final List<Scooter> _scooters = [];
  bool isLoading = true;
  bool areMarkersLoading = true;
  Scooter? currScooter;
  late Position currLoc;

  final ScooterAPI _scooterAPI = ScooterAPI();
  final AuthAPI _authAPI = AuthAPI();

  Timer? locationTimer;
  Timer? scooterTimer;

  void _onMapCreated(GoogleMapController controller) {
    mapController.complete(controller);

    setState(() {
      isLoading = false;
    });
    _getScooters();
    _initMarkers();
  }

  void _getScooters() async {
    http.Response response = await _scooterAPI.getScooters();
    var scooterObjsJson = jsonDecode(response.body) as List;
    for (var scooterJson in scooterObjsJson) {
      if (scooterJson['user'] == '_none') {
        if (scooterJson['location']['lat'] == null) {
          continue;
        }
        _scooters.add(Scooter(
          id: scooterJson['_id'],
          user: scooterJson['user'],
          lat: scooterJson['location']['lat'],
          long: scooterJson['location']['lon'],
          currentCharge: scooterJson['currentCharge'],
          scooterId: scooterJson['scooterId'],
        ));
      }
    }
    setState(() {});
    return;
  }

  void _initMarkers() async {
    List<Marker> generatedMarkers = [];
    final BitmapDescriptor markerImage = await BitmapDescriptor.fromAssetImage(
        const ImageConfiguration(),
        'lib/assets/upscaled_electric_scooter_icon.png');
    for (var scooter in _scooters) {
      generatedMarkers.add(Marker(
          markerId: MarkerId(scooter.id),
          position: LatLng(scooter.lat, scooter.long),
          icon: markerImage,
          onTap: () {
            Navigator.push(
              context,
              MaterialPageRoute(builder: (context) => ScooterPage(scooter)),
            );
          }));
    }
    setState(() {
      _markers = generatedMarkers.toSet();
      areMarkersLoading = false;
      _scooters.clear();
    });
  }

  @override
  void initState() {
    super.initState();
    locationTimer = Timer.periodic(const Duration(seconds: 7), (timer) async {
      currLoc = await Geolocator.getCurrentPosition(
          desiredAccuracy: LocationAccuracy.best);
      final res = await _authAPI.updateUserLocation(
          UserPreferences.getUser().username, currLoc);
      setState(() {
        if (res.body != '[]') {
          var scooterObjJson = jsonDecode(res.body) as List;
          if (currScooter == null) {
            isLoading = true;
          }
          currScooter = Scooter(
            id: scooterObjJson[0]["_id"],
            user: scooterObjJson[0]["user"],
            lat: scooterObjJson[0]['location']['lat'],
            long: scooterObjJson[0]['location']['lon'],
            currentCharge: scooterObjJson[0]['currentCharge'],
            scooterId: scooterObjJson[0]['scooterId'],
          );
          if (const lat_lng.Distance().as(
                  lat_lng.LengthUnit.Kilometer,
                  lat_lng.LatLng(currLoc.latitude, currLoc.longitude),
                  lat_lng.LatLng(scooterObjJson[0]['location']['lat'],
                      scooterObjJson[0]['location']['lon'])) >
              1) {
            isLoading = true;
            freeScooter();
          }
        }
        isLoading = false;
      });
    });
    //get scooter locations and refresh map every 7 seconds
    scooterTimer = Timer.periodic(const Duration(seconds: 7), (timer) async {
      _getScooters();
      _initMarkers();
    });
  }

  @override
  void dispose() {
    super.dispose();
    locationTimer?.cancel();
    scooterTimer?.cancel();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        appBar: AppBar(
          actions: [
            IconButton(
                onPressed: () {
                  Navigator.push(
                      context,
                      MaterialPageRoute(
                          builder: (context) => const ProfilePage()));
                },
                icon: const Icon(Icons.account_circle_rounded)),
          ],
        ),
        body: Stack(
          children: <Widget>[
            Opacity(
              opacity: isLoading ? 0.5 : 1,
              child: GoogleMap(
                initialCameraPosition: const CameraPosition(
                    target: LatLng(49.262986, -123.251995), zoom: 10),//UBC
                markers: _markers,
                myLocationEnabled: true,
                myLocationButtonEnabled: true,
                compassEnabled: true,
                onMapCreated: (GoogleMapController controller) {
                  _onMapCreated(controller);
                },
              ),
            ),
            currScooter != null
                ? Align(
                    alignment: Alignment.bottomCenter,
                    child: Card(
                        margin: const EdgeInsets.all(4),
                        child: Column(
                            mainAxisSize: MainAxisSize.min,
                            children: <Widget>[
                              ListTile(
                                leading: const Icon(
                                  Icons.electric_scooter,
                                  color: Colors.lightGreenAccent,
                                ),
                                title: Text(
                                    'Scooter ID: ${currScooter?.scooterId}'),
                                subtitle: const Text('Current Charge: 72%'),
                              ),
                              Row(
                                mainAxisAlignment: MainAxisAlignment.end,
                                children: <Widget>[
                                  TextButton(
                                    child: const Text('END TRIP'),
                                    onPressed: () {
                                      freeScooter();
                                    },
                                  ),
                                  const SizedBox(width: 8),
                                ],
                              ),
                            ])))
                : const SizedBox(),
            Opacity(
                opacity: isLoading ? 1 : 0,
                child: Center(
                  child: LoadingAnimationWidget.threeArchedCircle(
                      color: Colors.blue, size: 100),
                )),
          ],
        ));
  }







  //------------------------------------DIALOGS---------------------------------
  Future<void> _showTimeElapsed(String timeString) async {
    int milliseconds = int.parse(timeString);

    return showDialog<void>(
        context: context,
        barrierDismissible: true,
        builder: (BuildContext context) {
          return AlertDialog(
            title: const Text('Thank You'),
            content: SingleChildScrollView(
              child: ListBody(
                children: <Widget>[
                  Text(
                      'Your total elapsed time was : ${Duration(milliseconds: milliseconds).format()}\nYour trip costs ${NumberFormat.simpleCurrency().format((milliseconds/1000)/60 * 0.40)}.\nThank you for using VisiRide!'),
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
        });
  }

  Future<void> freeScooter() async {
    setState(() {
      isLoading = true;
    });
    var res = await AuthAPI().freeScooter(
        UserPreferences.getUser().username, currScooter!.scooterId);
    if (res.statusCode == 200) {
      _showTimeElapsed(res.body);
      setState(() {
        currScooter = null;
      });
    } else {
      _show400Err();
    }
    setState(() {
      isLoading = false;
    });
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
                  Text('Please try again.'),
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
        });
  }
}
extension on Duration{
  String format() => '$this'.split('.')[0].padLeft(8, '0');
}
