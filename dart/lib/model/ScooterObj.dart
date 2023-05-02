class Scooter{
  String id;
  String user;
  double lat;
  double long;
  int? currentCharge;
  String scooterId;

  Scooter({required this.id, required this.user, required this.lat, required this.long, required this.currentCharge, required this.scooterId});
  
  static Scooter fromJson(Map<String, dynamic> json) => Scooter(
      id: json['_id'],
      user: json['user'],
      lat: json['location']['lat'],
      long: json['location']['lon'],
    currentCharge: json['currentCharge'],
    scooterId: json['scooterId'],
  );
  Map<String, dynamic> toJson() =>{
    '_id': id,
    'user': user,
    'location': {
      'lat': lat,
      'lon': long,
    },
    'currentCharge':currentCharge,
    'scooterId':scooterId,
  };
}