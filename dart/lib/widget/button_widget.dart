import 'package:flutter/material.dart';

class ButtonWidget extends StatelessWidget {
  final String text;
  final VoidCallback onPressed;

  const ButtonWidget({
    Key? key,
    required this.text,
    required this.onPressed,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) => ElevatedButton(
    style: ElevatedButton.styleFrom(
      backgroundColor: const Color(0xffbb86fc),
      foregroundColor: Colors.white, shape: const StadiumBorder(),
      padding: const EdgeInsets.symmetric(horizontal: 32, vertical: 12),
    ),
    onPressed: onPressed,
    child: Text(text),
  );
}