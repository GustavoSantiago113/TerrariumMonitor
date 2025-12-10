import 'package:flutter/material.dart';
import 'package:terrarium_monitor_app/models/terrarium_reading.dart';
import 'package:terrarium_monitor_app/services/api_service.dart';
import 'package:intl/intl.dart';

class AppState extends ChangeNotifier {
  List<SensorReading> allReadings = [];
  List<ApiDataRecord> allApiRecords = [];
  ImageMonitor? latestImage;
  String selectedSensor = 'temperature';
  bool isFetching = true;
  int visibleCount = 8;
  int currentPage = 0;
  bool hasMorePages = true;
  bool isLoadingMore = false;

  Future<void> fetchData() async {
    isFetching = true;
    currentPage = 0;
    hasMorePages = true;
    allApiRecords.clear();
    notifyListeners();

    try {
      print('🔄 Starting to fetch data from API...');
      
      // 1) Fetch first page (page 0) for historical data
      final firstPageRecords = await ApiService.fetchData(page: 0);
      print('✅ Fetched ${firstPageRecords.length} records from page 0');

      if (firstPageRecords.isEmpty) {
        throw Exception('No data available from API');
      }

      allApiRecords = firstPageRecords;
      hasMorePages = firstPageRecords.length == 8; // Assuming 8 records per page

      // Keep image selection logic: pick the newest by createdAt
      final latestRecord = allApiRecords.reduce((a, b) => a.createdAt.isAfter(b.createdAt) ? a : b);
      latestImage = ImageMonitor(
        imageUrl: latestRecord.primaryImageUrl,
        date: latestRecord.formattedDate,
        time: latestRecord.formattedTime,
      );

      // 2) Fetch last-24h data for graphs only (smaller, server-filtered response)
      final last24h = await ApiService.fetchLast24h();
      print('✅ Fetched ${last24h.length} records (last 24h) for graphs');

      // Convert last24h records to SensorReading objects and sort chronologically
      allReadings = last24h.map((record) => record.toSensorReading()).toList()
        ..sort((a, b) => a.timestamp.compareTo(b.timestamp));

      // Use the entire last-24h dataset for graphs by default
      visibleCount = allReadings.length;

      print('✅ Successfully prepared ${allReadings.length} sensor readings for graphs');

    } catch (e, stackTrace) {
      print('❌ Error fetching data from API: $e');
      print('📋 Stack trace: $stackTrace');
      
      // No fallback - let UI show no data state
      allApiRecords.clear();
      allReadings.clear();
      latestImage = null;
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
  
  /// Load next page of historical data
  Future<void> loadNextPage() async {
    if (isLoadingMore || !hasMorePages) return;
    
    isLoadingMore = true;
    notifyListeners();
    
    try {
      currentPage++;
      final nextPageRecords = await ApiService.fetchData(page: currentPage);
      print('✅ Fetched ${nextPageRecords.length} records from page $currentPage');
      
      if (nextPageRecords.isEmpty) {
        hasMorePages = false;
        currentPage--; // Revert page number
      } else {
        allApiRecords.addAll(nextPageRecords);
        hasMorePages = nextPageRecords.length == 8; // Assuming 8 records per page
      }
    } catch (e) {
      print('❌ Error loading next page: $e');
      currentPage--; // Revert page number on error
    }
    
    isLoadingMore = false;
    notifyListeners();
  }
}