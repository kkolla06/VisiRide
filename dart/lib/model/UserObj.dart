class User {
  final String image64;
  final String username;
  final String password;

  const User({
    required this.image64,
    required this.username,
    required this.password,
  });

  User copy({
    String? image64,
    String? username,
    String? password,
  }) =>
      User(
        image64: image64 ?? this.image64,
        username: username ?? this.username,
        password: password ?? this.password,
      );

  static User fromJson(Map<String, dynamic> json) => User(
    image64: json['image64'],
    username: json['username'],
    password: json['password'],
  );

  Map<String, dynamic> toJson() => {
    'image64': image64,
    'username': username,
    'password': password,
  };
}