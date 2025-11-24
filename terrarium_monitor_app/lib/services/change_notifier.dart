import 'package:flutter/material.dart';
import 'package:terrarium_monitor_app/models/terrarium_reading.dart';
import 'package:terrarium_monitor_app/services/api_service.dart';

class AppState extends ChangeNotifier {
  List<SensorReading> allReadings = [];
  List<ApiDataRecord> allApiRecords = [];
  ImageMonitor? latestImage;
  String selectedSensor = 'temperature';
  bool isFetching = true;
  int visibleCount = 8;

  Future<void> fetchData() async {
    isFetching = true;
    notifyListeners();

    try {
      // Fetch all data from API (will fetch multiple pages until empty)
      allApiRecords = await ApiService.fetchAllData(maxPages: 20);
      
      if (allApiRecords.isEmpty) {
        throw Exception('No data available');
      }

      // Convert API records to SensorReading objects
      allReadings = allApiRecords
          .map((record) => record.toSensorReading())
          .toList()
        ..sort((a, b) => a.timestamp.compareTo(b.timestamp)); // Sort by time

      // Get the latest record for the image
      final latestRecord = allApiRecords.first; // API returns newest first
      
      latestImage = ImageMonitor(
        imageUrl: latestRecord.primaryImageUrl,
        date: latestRecord.formattedDate,
        time: latestRecord.formattedTime,
      );

      // Initialize visibleCount to a sensible default (don't exceed available readings)
      visibleCount = allReadings.length < visibleCount ? allReadings.length : visibleCount;

    } catch (e) {
      print('Error fetching data: $e');
      latestImage = ImageMonitor(
        imageUrl: 'https://placehold.co/400x250/E0E0E0/6C6C6C?text=Image+Not+Found',
        date: 'N/A',
        time: 'N/A',
      );
    }

    isFetching = false;
    notifyListeners();
  }

  void updateSelectedSensor(String sensor) {
    selectedSensor = sensor;
    notifyListeners();
  }

  /// Returns the most recent [visibleCount] readings in chronological order.
  List<SensorReading> get visibleReadings {
    if (allReadings.isEmpty) return [];
    if (visibleCount <= 0) return [];
    if (allReadings.length <= visibleCount) return List<SensorReading>.from(allReadings);
    return allReadings.sublist(allReadings.length - visibleCount);
  }

  /// Update how many recent readings should be considered "visible" and
  /// notify listeners so dependent UI (like graphs) can update.
  void setVisibleCount(int n) {
    final newCount = n.clamp(0, allReadings.length);
    if (newCount != visibleCount) {
      visibleCount = newCount;
      notifyListeners();
    }
  }
}