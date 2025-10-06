import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_storage/firebase_storage.dart';
import 'package:flutter/material.dart';
import 'package:terrarium_monitor_app/models/terrarium_reading.dart';
import 'package:intl/intl.dart';

class AppState extends ChangeNotifier {
  List<SensorReading> allReadings = [];
  ImageMonitor? latestImage;
  String selectedSensor = 'temperature';
  bool isFetching = true;

  final String _databasePath = 'terrarium_monitor_images';

  Future<void> fetchData() async {
    isFetching = true;
    notifyListeners();

    try {
      final dbRef = FirebaseDatabase.instance.ref(_databasePath);
      final snapshot = await dbRef.once();

      final data = snapshot.snapshot.value as Map<dynamic, dynamic>?;
      if (data == null || data.isEmpty) {
        throw Exception('No data available');
      }

      // --- Fetch all sensor readings ---
      final tempReadings = data['temp'] as Map<dynamic, dynamic>? ?? {};
      final humiReadings = data['humi'] as Map<dynamic, dynamic>? ?? {};
      final luxReadings = data['lux'] as Map<dynamic, dynamic>? ?? {};

      final allKeys = <String>{
        ...tempReadings.keys.cast<String>(),
        ...humiReadings.keys.cast<String>(),
        ...luxReadings.keys.cast<String>(),
      };

      allReadings = allKeys.map((key) {
        final timestamp = DateTime.fromMillisecondsSinceEpoch(int.parse(key) * 1000);
        return SensorReading(
          temperature: (tempReadings[key] as num?)?.toDouble() ?? 0.0,
          humidity: (humiReadings[key] as num?)?.toDouble() ?? 0.0,
          light: (luxReadings[key] as num?)?.toDouble() ?? 0.0,
          timestamp: timestamp,
        );
      }).toList()
        ..sort((a, b) => a.timestamp.compareTo(b.timestamp)); // Sort by time

      String? latestKey;
      if (allKeys.isNotEmpty) {
        latestKey = allKeys.reduce((a, b) => int.parse(a) > int.parse(b) ? a : b);
      }

      // --- Fetch latest image with error handling ---
      if (latestKey != null) {
        try {
          final latestFileRef = FirebaseStorage.instance.ref().child('$_databasePath/$latestKey.jpg');
          final imageUrl = await latestFileRef.getDownloadURL();

          final timestamp = DateTime.fromMillisecondsSinceEpoch(int.parse(latestKey) * 1000);
          final formattedDate = DateFormat('yyyy-MM-dd').format(timestamp);
          final formattedTime = DateFormat('HH:mm').format(timestamp);

          latestImage = ImageMonitor(
            imageUrl: imageUrl,
            date: formattedDate,
            time: formattedTime,
          );
        } on FirebaseException catch (e) {
          print('Error fetching latest image: $e');
          latestImage = ImageMonitor(
            imageUrl: 'https://placehold.co/400x250/E0E0E0/6C6C6C?text=Image+Not+Found',
            date: 'N/A',
            time: 'N/A',
          );
        }
      } else {
        latestImage = null;
      }

    } catch (e) {
      print('Error fetching data: $e');
    }

    isFetching = false;
    notifyListeners();
  }

  void updateSelectedSensor(String sensor) {
    selectedSensor = sensor;
    notifyListeners();
  }
}