class BaseAPI{
  static String base = "http://20.238.80.71:8081";
  static var api = base;
  var customersPath = "$api/users";
  var signUpPath = "$api/sign_up";
  var authPath = "$api/sign_in";
  var logoutPath = "$api/logout";
  var scootersPath = '$api/scooters';
  var updateUserLocPath = '$api/update_user_location';
  var freeScooterPath = "$api/free_scooter";

  Map<String, String> headers = {
    "Content-Type": "application/json; charset=UTF-8"};
}