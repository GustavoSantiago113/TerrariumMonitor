import 'package:flutter/material.dart';
import 'package:terrarium_monitor_app/services/change_notifier.dart';
import 'package:terrarium_monitor_app/components/build_metrics_row.dart';


Widget buildMetricsCard(AppState appState) {
  final latestReading = appState.allReadings.isNotEmpty ? appState.allReadings.last : null;

  return Padding(
    padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
    child: Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        buildMetricRow(
          icon: Icons.thermostat,
          value: latestReading != null ? '${latestReading.temperature.toStringAsFixed(0)}°C' : '--',
          color: Colors.red,
        ),
        buildMetricRow(
          icon: Icons.water_drop,
          value: latestReading != null ? '${latestReading.humidity.toStringAsFixed(0)}%' : '--',
          color: Colors.blue,
        ),
        buildMetricRow(
          icon: Icons.lightbulb_outline,
          value: latestReading != null ? '${latestReading.light.toStringAsFixed(0)}' : '--',
          color: Colors.amber,
        ),
      ],
    ),
  );
}