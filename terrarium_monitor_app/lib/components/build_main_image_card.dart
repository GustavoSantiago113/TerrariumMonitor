import 'package:flutter/material.dart';
import 'package:terrarium_monitor_app/models/terrarium_reading.dart';

Widget buildMainImageCard(ImageMonitor latestImage) {
  final isPlaceholder = latestImage.imageUrl.contains('placehold.co');

  return Card(
    margin: const EdgeInsets.symmetric(horizontal: 16),
    elevation: 4,
    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
    child: Stack(
      alignment: Alignment.bottomRight,
      children: [
        ClipRRect(
          borderRadius: BorderRadius.circular(16),
          child: isPlaceholder
              ? Container(
                  width: double.infinity,
                  height: 250,
                  color: const Color(0xFFD9D9D9),
                  child: const Center(
                    child: Text(
                      'PHOTO',
                      style: TextStyle(
                        fontSize: 36,
                        fontWeight: FontWeight.bold,
                        color: Colors.black38,
                        letterSpacing: 2,
                      ),
                    ),
                  ),
                )
              : Image.network(
                  latestImage.imageUrl,
                  fit: BoxFit.cover,
                  width: double.infinity,
                  height: 250,
                  loadingBuilder: (context, child, loadingProgress) {
                    if (loadingProgress == null) return child;
                    return const Center(child: CircularProgressIndicator());
                  },
                  errorBuilder: (context, error, stackTrace) => Container(
                    width: double.infinity,
                    height: 250,
                    color: const Color(0xFFD9D9D9),
                    child: const Center(
                      child: Text(
                        'PHOTO',
                        style: TextStyle(
                          fontSize: 36,
                          fontWeight: FontWeight.bold,
                          color: Colors.black38,
                          letterSpacing: 2,
                        ),
                      ),
                    ),
                  ),
                ),
        ),
        Padding(
          padding: const EdgeInsets.all(12.0),
          child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
            decoration: BoxDecoration(
              color: Colors.black.withOpacity(0.5),
              borderRadius: BorderRadius.circular(12),
            ),
            child: Text(
              '${latestImage.date}\n${latestImage.time}',
              style: const TextStyle(
                color: Colors.white,
                fontSize: 14,
                fontWeight: FontWeight.bold,
              ),
              textAlign: TextAlign.right,
            ),
          ),
        ),
      ],
    ),
  );
}