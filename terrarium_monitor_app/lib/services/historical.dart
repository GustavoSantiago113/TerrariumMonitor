import 'package:flutter/material.dart';
import 'package:terrarium_monitor_app/models/terrarium_reading.dart';
import 'package:terrarium_monitor_app/services/api_service.dart';
import 'package:terrarium_monitor_app/services/change_notifier.dart';
import 'package:terrarium_monitor_app/components/build_main_image_card.dart';


class HistorySection extends StatefulWidget {
  final AppState appState;
  final int maxItems;
  const HistorySection({super.key, required this.appState, this.maxItems = 8});

  @override
  State<HistorySection> createState() => _HistorySectionState();
}

class _HistorySectionState extends State<HistorySection> {
  static const String _placeholder =
      'https://placehold.co/400x250/E0E0E0/6C6C6C?text=Image+Not+Found';

  void _openDetail(BuildContext context, ApiDataRecord record) {
    final imageMonitor = ImageMonitor(
      imageUrl: record.primaryImageUrl,
      date: record.formattedDate,
      time: record.formattedTime,
    );

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
                  Text('Temperature: ${record.temperature.toStringAsFixed(2)}°C',
                      style: const TextStyle(fontSize: 16, color: Colors.white)),
                  const SizedBox(height: 6),
                  Text('Humidity: ${record.humidity.toStringAsFixed(2)}%',
                      style: const TextStyle(fontSize: 16, color: Colors.white)),
                  const SizedBox(height: 6),
                  Text('Light: ${record.luminosity.toStringAsFixed(0)}',
                      style: const TextStyle(fontSize: 16, color: Colors.white)),
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
    // Load next page from API instead of just showing more local items
    widget.appState.loadNextPage();
  }

  @override
  Widget build(BuildContext context) {
    final apiRecords = widget.appState.allApiRecords;
    if (apiRecords.isEmpty) {
      return const SizedBox.shrink();
    }

    // Show all available API records (will grow as user loads more pages)
    final items = apiRecords;

    final hasMore = widget.appState.hasMorePages;

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
                      child: Center(
                        child: widget.appState.isLoadingMore
                            ? const SizedBox(
                                width: 20,
                                height: 20,
                                child: CircularProgressIndicator(
                                  strokeWidth: 2,
                                  valueColor: AlwaysStoppedAnimation<Color>(Colors.white),
                                ),
                              )
                            : const Text(
                                'SEE MORE',
                                style: TextStyle(
                                  fontSize: 14,
                                  color: Colors.white,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                      ),
                    ),
                  ),
                );
              }

              final record = items[index];
              final imageUrl = record.primaryImageUrl;
              final isPlaceholder = imageUrl == _placeholder;

              return Padding(
                padding: const EdgeInsets.only(right: 12.0),
                child: GestureDetector(
                  onTap: () {
                    _openDetail(context, record);
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
                ),
              );
            },
          ),
        ),
      ],
    );
  }
}