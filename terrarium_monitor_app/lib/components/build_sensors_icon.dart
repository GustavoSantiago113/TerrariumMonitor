import 'package:flutter/material.dart';
import 'package:terrarium_monitor_app/services/change_notifier.dart';

Widget _buildSensorIcon({
  required IconData icon,
  required String label,
  required Color color,
  required bool isSelected,
  required VoidCallback onTap,
}) {
  return GestureDetector(
    onTap: onTap,
    child: Column(
      children: [
        Container(
          padding: const EdgeInsets.all(12),
          decoration: BoxDecoration(
            color: isSelected ? color.withOpacity(0.2) : Colors.transparent,
            shape: BoxShape.circle,
            border: Border.all(color: isSelected ? color : Colors.grey),
          ),
          child: Icon(icon, color: isSelected ? color : Colors.grey, size: 30),
        ),
        const SizedBox(height: 4),
        Text(
          label,
          style: TextStyle(
            fontSize: 12,
            color: isSelected ? color : Colors.grey,
          ),
        ),
      ],
    ),
  );
}

Widget buildSensorIcons(AppState appState) {
  return Row(
    mainAxisAlignment: MainAxisAlignment.spaceAround,
    children: [
      _buildSensorIcon(
        icon: Icons.thermostat,
        label: 'Temperature',
        color: Colors.red,
        isSelected: appState.selectedSensor == 'temperature',
        onTap: () => appState.updateSelectedSensor('temperature'),
      ),
      _buildSensorIcon(
        icon: Icons.water_drop,
        label: 'Humidity',
        color: Colors.blue,
        isSelected: appState.selectedSensor == 'humidity',
        onTap: () => appState.updateSelectedSensor('humidity'),
      ),
      _buildSensorIcon(
        icon: Icons.lightbulb_outline,
        label: 'Light',
        color: Colors.amber,
        isSelected: appState.selectedSensor == 'light',
        onTap: () => appState.updateSelectedSensor('light'),
      ),
    ],
  );
}