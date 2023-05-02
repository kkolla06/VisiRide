import 'dart:convert';
import 'dart:io';
import 'package:geolocator/geolocator.dart';

import '/api/BaseAPI.dart';
import 'package:http/http.dart' as http;


class AuthAPI extends BaseAPI{
  Future<http.Response> signUp(String username,
      String password, File face, Position pos) async {
    List<int> imgBytes = face.readAsBytesSync();
    var face64 = base64Encode(imgBytes);
    var body = jsonEncode({
      'username': username,
      'password': password,
      'image': face64,
      'lat': pos.latitude,
      'lon': pos.longitude,
    });
    http.Response response = await http.post(
        Uri.parse(super.signUpPath),
        headers: super.headers,
        body: body);

    return response;
  }
  Future<http.Response> login(String username, String password) async{
    var queryParameters = {
      'username': username,
      'password': password,
    };
    var uri = Uri.http('20.238.80.71:8081', '/sign_in', queryParameters);
    http.Response response =
        await http.get(uri);
    return response;

  }
  Future<http.Response> logout(int id, String token) async {
    var body = jsonEncode({'id':id, 'token':token});

    http.Response response = await http.post(Uri.parse(super.logoutPath), body: body);
    return response;
  }

  Future<http.Response> updateUserLocation(String username, Position pos) async{
    var body = jsonEncode({'username': username, 'lat': pos.latitude, 'lon':pos.longitude});
    http.Response response = await http.post(Uri.parse(super.updateUserLocPath),headers: super.headers, body: body);

    return response;
  }

  Future<http.Response> freeScooter(String username, String scooterId) async{
    var body = jsonEncode({'username': username, 'scooterId': scooterId});
    http.Response response = await http.post(Uri.parse(super.freeScooterPath), headers: super.headers, body: body);

    return response;
  }
 }
