import 'dart:convert';
import 'package:http/http.dart' as http;
import 'package:terrarium_monitor_app/models/terrarium_reading.dart';
import 'package:intl/intl.dart';
import 'package:flutter_dotenv/flutter_dotenv.dart';

class ApiService {
  static final String baseUrl = dotenv.env['BASE_URL']!;
  static final String apiKey = dotenv.env['API_KEY']!;

  /// Fetch data from the API for a specific page
  /// Returns a list of ApiDataRecord objects
  static Future<List<ApiDataRecord>> fetchData({int page = 0}) async {
    try {
      final url = Uri.parse('$baseUrl/api/data?page=$page');
      final response = await http.get(
        url,
        headers: {
          'X-API-Key': apiKey,
        },
      );

      if (response.statusCode == 200) {
        if (response.body.isEmpty) {
          print('API returned an empty response.');
          return [];
        }

        final dynamic decodedBody = json.decode(response.body);
        if (decodedBody is! List) {
          throw Exception('Unexpected response format: ${response.body}');
        }

        final List<dynamic> jsonList = decodedBody;
        return jsonList.map((json) => ApiDataRecord.fromJson(json)).toList();
      } else {
        throw Exception('Failed to load data: ${response.statusCode}');
      }
    } catch (e) {
      print('Error fetching data from API: $e');
      rethrow;
    }
  }

  /// Fetch all available pages (useful for getting complete history)
  static Future<List<ApiDataRecord>> fetchAllData({int maxPages = 10}) async {
    List<ApiDataRecord> allRecords = [];
    
    for (int page = 0; page < maxPages; page++) {
      try {
        final records = await fetchData(page: page);
        if (records.isEmpty) {
          break; // No more data
        }
        allRecords.addAll(records);
      } catch (e) {
        print('Error fetching page $page: $e');
        break;
      }
    }
    
    return allRecords;
  }

  /// Get the full image URL from a path
  static String getImageUrl(String imagePath) {
    if (imagePath.startsWith('http')) {
      return imagePath;
    }
    return '$baseUrl$imagePath';
  }
}

/// Model for API data record
class ApiDataRecord {
  final String id;
  final String timestamp;
  final double luminosity;
  final double temperature;
  final double humidity;
  final List<String> imagePaths;
  final DateTime createdAt;

  ApiDataRecord({
    required this.id,
    required this.timestamp,
    required this.luminosity,
    required this.temperature,
    required this.humidity,
    required this.imagePaths,
    required this.createdAt,
  });

  factory ApiDataRecord.fromJson(Map<String, dynamic> json) {
    return ApiDataRecord(
      id: json['id'] as String,
      timestamp: json['timestamp'] as String,
      luminosity: (json['luminosity'] as num).toDouble(),
      temperature: (json['temperature'] as num).toDouble(),
      humidity: (json['humidity'] as num).toDouble(),
      imagePaths: (json['image_paths'] as List<dynamic>? ?? [])
          .map((e) => e as String)
          .toList(),
      createdAt: DateTime.parse(json['created_at'] as String),
    );
  }

  /// Convert to SensorReading for compatibility with existing code
  SensorReading toSensorReading() {
    return SensorReading(
      temperature: temperature,
      humidity: humidity,
      light: luminosity,
      timestamp: createdAt,
    );
  }

  /// Get the first image URL (or placeholder if none)
  String get primaryImageUrl {
    if (imagePaths.isEmpty) {
      return 'https://placehold.co/400x250/E0E0E0/6C6C6C?text=Image+Not+Found';
    }
    return ApiService.getImageUrl(imagePaths.first);
  }

  /// Get formatted date string
  String get formattedDate {
    return DateFormat('yyyy-MM-dd').format(createdAt);
  }

  /// Get formatted time string
  String get formattedTime {
    return DateFormat('HH:mm').format(createdAt);
  }
}
