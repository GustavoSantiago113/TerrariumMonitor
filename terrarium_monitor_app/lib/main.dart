import 'dart:async';
import 'package:firebase_core/firebase_core.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:terrarium_monitor_app/pages/home_screen.dart';
import 'package:terrarium_monitor_app/services/change_notifier.dart';
import 'firebase_options.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter_dotenv/flutter_dotenv.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await dotenv.load(fileName: ".env");
 
  await Firebase.initializeApp(
    options: DefaultFirebaseOptions.currentPlatform,
  );
  
  try {
    final email = dotenv.env['EMAIL']!;
    final password = dotenv.env['PASSWORD']!;
    try {
      final userCred = await FirebaseAuth.instance.signInWithEmailAndPassword(
        email: email,
        password: password,
      );
      print('Signed in uid: ${userCred.user?.uid}');
    } catch (e) {
      // Check if user is actually signed in despite the error
      final currentUser = FirebaseAuth.instance.currentUser;
      if (currentUser != null) {
        print('Sign-in succeeded despite error. UID: ${currentUser.uid}');
      } else {
        print('Sign-in failed: $e');
        rethrow;
      }
    }

    runApp(
      ChangeNotifierProvider(
        create: (_) => AppState()..fetchData(),
        child: MaterialApp(
          debugShowCheckedModeBanner: false,
          initialRoute: '/',
          routes: {
            '/': (context) => const Home(),
          },
        ),
      ),
    );
  } on FirebaseAuthException catch (e) {
    print('Failed to sign in: ${e.message}');
    runApp(
      const MaterialApp(
        debugShowCheckedModeBanner: false,
        home: Scaffold(
          body: Center(
            child: Text('Authentication Failed. Please check your credentials.'),
          ),
        ),
      ),
    );
  }
}