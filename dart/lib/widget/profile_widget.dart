import 'dart:convert';
import 'package:flutter/material.dart';

class ProfileWidget extends StatelessWidget {
  final String image64;

  const ProfileWidget({
    Key? key,
    required this.image64,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {

    return Center(
      child: Stack(
        children: [
          buildImage(),
        ],
      ),
    );
  }

  Widget buildImage() {
    final image = Image.memory(base64Decode(image64));

    return ClipOval(
      child: Material(
        color: Colors.transparent,
        child: Ink.image(
          image: image.image,
          fit: BoxFit.cover,
          width: 128,
          height: 128,
          child: const InkWell(),
        ),
      ),
    );
  }

  Widget buildCircle({
    required Widget child,
    required double all,
    required Color color,
  }) =>
      ClipOval(
        child: Container(
          padding: EdgeInsets.all(all),
          color: color,
          child: child,
        ),
      );
}