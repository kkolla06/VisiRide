import '/api/BaseAPI.dart';
import 'package:http/http.dart' as http;

class ScooterAPI extends BaseAPI{
  Future<http.Response> getScooters() async{
    http.Response response = await http.get(Uri.parse(super.scootersPath), );

    return response;
  }

}