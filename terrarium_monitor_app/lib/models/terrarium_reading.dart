class SensorReading {
  final double temperature;
  final double humidity;
  final double light;
  final DateTime timestamp;

  SensorReading({
    required this.temperature,
    required this.humidity,
    required this.light,
    required this.timestamp,
  });
}

// Data model for the latest image
class ImageMonitor {
  final String imageUrl;
  final String date;
  final String time;

  ImageMonitor({
    required this.imageUrl,
    required this.date,
    required this.time,
  });
}