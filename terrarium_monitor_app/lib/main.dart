import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:terrarium_monitor_app/pages/home_screen.dart';
import 'package:terrarium_monitor_app/services/change_notifier.dart';
import 'package:flutter_dotenv/flutter_dotenv.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await dotenv.load(fileName: ".env");

  // Initialize AppState first to avoid providing a changing child
  final appState = AppState();
  await appState.fetchData();

  runApp(
    ChangeNotifierProvider.value(
      value: appState,
      child: const MyApp(),
    ),
  );
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      initialRoute: '/',
      routes: {
        '/': (context) => const Home(),
      },
    );
  }
}