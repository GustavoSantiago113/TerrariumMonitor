import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:terrarium_monitor_app/services/change_notifier.dart';
import 'package:terrarium_monitor_app/components/build_sensors_icon.dart';

List<double> _getGraphData(AppState appState) {
  final dataSource = appState.visibleReadings;
  switch (appState.selectedSensor) {
    case 'temperature':
      return dataSource.map((r) => r.temperature).toList();
    case 'humidity':
      return dataSource.map((r) => r.humidity).toList();
    case 'light':
      return dataSource.map((r) => r.light).toList();
    default:
      return [];
  }
}

double _getMinY(String sensor, List<double> data) {
  if (data.isEmpty) return 0;
  double min = data.reduce((a, b) => a < b ? a : b);
  return min - (min * 0.1); // 10% buffer
}

double _getMaxY(String sensor, List<double> data) {
  if (data.isEmpty) return 1;
  double max = data.reduce((a, b) => a > b ? a : b);
  return max + (max * 0.1); // 10% buffer
}

Color _getGraphColor(String sensor) {
  switch (sensor) {
    case 'temperature':
      return Colors.red;
    case 'humidity':
      return Colors.blue;
    case 'light':
      return Colors.amber;
    default:
      return Colors.grey;
  }
}

Widget buildGraphSection(AppState appState) {

    final data = _getGraphData(appState);

    return Card(
      margin: const EdgeInsets.symmetric(horizontal: 16),
      elevation: 4,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            buildSensorIcons(appState),
            const SizedBox(height: 16),
            Container(
              height: 250,
              padding: const EdgeInsets.symmetric(horizontal: 8),
              child: LineChart(
                LineChartData(
                  minX: 0,
                  maxX: data.length > 1 ? (data.length - 1).toDouble() : 1,
                  minY: _getMinY(appState.selectedSensor, data),
                  maxY: _getMaxY(appState.selectedSensor, data),
                  titlesData: FlTitlesData(
                    leftTitles: AxisTitles(
                      sideTitles: SideTitles(
                        showTitles: true,
                        reservedSize: 40,
                        getTitlesWidget: (value, meta) {
                          return Text(value.toStringAsFixed(1), style: const TextStyle(fontSize: 12));
                        },
                      ),
                    ),
                    bottomTitles: AxisTitles(
                      sideTitles: SideTitles(showTitles: false),
                    ),
                    topTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                    rightTitles: AxisTitles(sideTitles: SideTitles(showTitles: false)),
                  ),
                  gridData: FlGridData(
                    show: true,
                    drawHorizontalLine: true,
                    drawVerticalLine: false,
                    getDrawingHorizontalLine: (value) {
                      return FlLine(color: Colors.grey.withOpacity(0.2), strokeWidth: 1);
                    },
                  ),
                  borderData: FlBorderData(
                    show: true,
                    border: Border.all(color: Colors.grey.withOpacity(0.2)),
                  ),
                  lineBarsData: [
                    LineChartBarData(
                      spots: data.asMap().entries.map((entry) {
                        return FlSpot(entry.key.toDouble(), entry.value);
                      }).toList(),
                      isCurved: true,
                      color: _getGraphColor(appState.selectedSensor),
                      barWidth: 2,
                      isStrokeCapRound: true,
                      dotData: FlDotData(show: false),
                      belowBarData: BarAreaData(
                        show: true,
                        color: _getGraphColor(appState.selectedSensor).withOpacity(0.2),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }