// ...existing code...
import 'package:flutter/material.dart';
import 'package:firebase_storage/firebase_storage.dart';
import 'package:terrarium_monitor_app/models/terrarium_reading.dart';
import 'package:terrarium_monitor_app/services/change_notifier.dart';
import 'package:terrarium_monitor_app/components/build_main_image_card.dart';
import 'package:intl/intl.dart';

class HistorySection extends StatefulWidget {
  final AppState appState;
  final int maxItems;
  const HistorySection({super.key, required this.appState, this.maxItems = 8});

  @override
  State<HistorySection> createState() => _HistorySectionState();
}

class _HistorySectionState extends State<HistorySection> {
  final Map<String, Future<String>> _urlCache = {};
  static const String _placeholder =
      'https://placehold.co/400x250/E0E0E0/6C6C6C?text=Image+Not+Found';

  late int _itemsToShow;

  @override
  void initState() {
    super.initState();
    _itemsToShow = widget.maxItems;
  }

  Future<String> _imageUrlForKey(String key) {
    return _urlCache.putIfAbsent(key, () async {
      try {
        final ref = FirebaseStorage.instance.ref().child('terrarium_monitor_images/$key.jpg');
        final url = await ref.getDownloadURL();
        return url;
      } catch (e) {
        return _placeholder;
      }
    });
  }

  void _openDetail(BuildContext context, SensorReading reading, String imageUrl) {
    final dt = reading.timestamp;
    final date = DateFormat('yyyy-MM-dd').format(dt);
    final time = DateFormat('HH:mm').format(dt);

    final imageMonitor = ImageMonitor(imageUrl: imageUrl, date: date, time: time);

    showDialog(
      context: context,
      builder: (_) => Dialog(
        backgroundColor: const Color.fromARGB(255, 60, 68, 75),
        insetPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 24),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const SizedBox(height: 16),
            buildMainImageCard(imageMonitor),
            Padding(
              padding: const EdgeInsets.symmetric(vertical: 12.0, horizontal: 16),
              child: Column(
                children: [
                  Text('Temperature: ${reading.temperature.toStringAsFixed(2)}°C',
                      style: const TextStyle(fontSize: 16, color: Colors.white, fontFamily: 'Montserrat',)),
                  const SizedBox(height: 6),
                  Text('Humidity: ${reading.humidity.toStringAsFixed(2)}%',
                      style: const TextStyle(fontSize: 16, color: Colors.white, fontFamily: 'Montserrat',)),
                  const SizedBox(height: 6),
                  Text('Light: ${reading.light.toStringAsFixed(0)}',
                      style: const TextStyle(fontSize: 16, color: Colors.white, fontFamily: 'Montserrat',)),
                ],
              ),
            ),
            Align(
              alignment: Alignment.bottomRight,
              child: TextButton(
                onPressed: () => Navigator.of(context).pop(),
                child: const Text('Close'),
              ),
            ),
          ],
        ),
      ),
    );
  }

  void _seeMore() {
    final total = widget.appState.allReadings.length;
    if (_itemsToShow >= total) return;
    final next = _itemsToShow + widget.maxItems;
    setState(() {
      _itemsToShow = next > total ? total : next;
    });
  }

  @override
  Widget build(BuildContext context) {
    final readings = widget.appState.allReadings;
    if (readings.isEmpty) {
      return const SizedBox.shrink();
    }

    // Get the last _itemsToShow readings (latest first)
    final start = readings.length - _itemsToShow;
    final items = readings.sublist(start < 0 ? 0 : start).reversed.toList();

    final hasMore = _itemsToShow < readings.length;

    return Column(
      children: [
        const SizedBox(height: 12),
        SizedBox(
          height: 120,
          child: ListView.builder(
            scrollDirection: Axis.horizontal,
            padding: const EdgeInsets.symmetric(horizontal: 16),
            itemCount: items.length + (hasMore ? 1 : 0),
            itemBuilder: (context, index) {
              // If this is the "See more" tile
              if (index == items.length && hasMore) {
                return Padding(
                  padding: const EdgeInsets.only(right: 12.0),
                  child: GestureDetector(
                    onTap: _seeMore,
                    child: Container(
                      width: 100,
                      decoration: BoxDecoration(
                        color: const Color.fromARGB(255, 5, 152, 158),
                        borderRadius: BorderRadius.circular(12),
                        boxShadow: [
                          BoxShadow(
                            color: Colors.black.withOpacity(0.06),
                            blurRadius: 4,
                            offset: const Offset(0, 2),
                          ),
                        ],
                      ),
                      child: const Center(
                        child: Text(
                          'SEE MORE',
                          style: TextStyle(
                            fontSize: 14,
                            color: Colors.white,
                            fontWeight: FontWeight.bold,
                            fontFamily: 'Montserrat',
                          ),
                        ),
                      ),
                    ),
                  ),
                );
              }

              final reading = items[index];
              final key = (reading.timestamp.millisecondsSinceEpoch ~/ 1000).toString();

              return Padding(
                padding: const EdgeInsets.only(right: 12.0),
                child: FutureBuilder<String>(
                  future: _imageUrlForKey(key),
                  builder: (context, snap) {
                    final imageUrl = snap.data ?? _placeholder;
                    final isPlaceholder = imageUrl == _placeholder;

                    return GestureDetector(
                      onTap: () {
                        _openDetail(context, reading, imageUrl);
                      },
                      child: Container(
                        width: 100,
                        decoration: BoxDecoration(
                          color: const Color.fromARGB(255, 60, 68, 75),
                          borderRadius: BorderRadius.circular(12),
                          boxShadow: [
                            BoxShadow(
                              color: Colors.black.withOpacity(0.06),
                              blurRadius: 4,
                              offset: const Offset(0, 2),
                            ),
                          ],
                        ),
                        child: ClipRRect(
                          borderRadius: BorderRadius.circular(12),
                          child: isPlaceholder
                              ? const Center(
                                  child: Text(
                                    'PHOTO',
                                    style: TextStyle(
                                      fontSize: 14,
                                      color: Colors.black38,
                                      fontWeight: FontWeight.bold,
                                    ),
                                  ),
                                )
                              : Image.network(
                                  imageUrl,
                                  fit: BoxFit.cover,
                                  loadingBuilder: (ctx, child, progress) {
                                    if (progress == null) return child;
                                    return const Center(child: CircularProgressIndicator(strokeWidth: 2));
                                  },
                                  errorBuilder: (_, __, ___) => const Center(
                                    child: Text(
                                      'PHOTO',
                                      style: TextStyle(
                                        fontSize: 14,
                                        color: Colors.black38,
                                        fontWeight: FontWeight.bold,
                                      ),
                                    ),
                                  ),
                                ),
                        ),
                      ),
                    );
                  },
                ),
              );
            },
          ),
        ),
      ],
    );
  }
}