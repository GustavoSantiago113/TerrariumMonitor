import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:terrarium_monitor_app/components/build_graph_section.dart';
import 'package:terrarium_monitor_app/components/build_main_image_card.dart';
import 'package:terrarium_monitor_app/components/build_metrics_card.dart';

import 'package:terrarium_monitor_app/components/no_data.dart';
import 'package:terrarium_monitor_app/services/change_notifier.dart';
import 'package:terrarium_monitor_app/services/historical.dart';

class Home extends StatefulWidget{
  const Home({super.key});

  @override
  _HomeState createState() => _HomeState();
}

class _HomeState extends State<Home> {
  
  @override
  Widget build(BuildContext context) {
    final appState = Provider.of<AppState>(context);

    return Scaffold(
        backgroundColor: const Color.fromARGB(255, 33, 33, 33),
        appBar: PreferredSize(
          preferredSize: const Size.fromHeight(70),
          child: AppBar(
            backgroundColor: const Color.fromARGB(255, 5, 152, 158),
            centerTitle: true,
            elevation: 1,
            title: const Text(
              'Terrarium Monitor',
              style: TextStyle(
                fontFamily: 'Montserrat',
                fontSize: 30,
                color: Colors.white,
                letterSpacing: 1.2,
              ),
              textAlign: TextAlign.center,
            ),
          ),
        ),
        body: RefreshIndicator(
          onRefresh: () async {
            await appState.fetchData();
          },
          color: const Color(0xFF05989E),
          child: SingleChildScrollView(
            physics: const AlwaysScrollableScrollPhysics(),
            child: appState.isFetching
                // Keep a scrollable area while loading so pull-to-refresh works
                ? SizedBox(
                    height: MediaQuery.of(context).size.height - kToolbarHeight,
                    child: const Center(child: CircularProgressIndicator()),
                  )
                : appState.latestImage == null
                    ? noData()
                    : Column(
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        const SizedBox(height: 16),
                        const Text(
                          'Last Observation',
                          style: TextStyle(fontSize: 25, fontFamily: 'Montserrat', color: Color.fromARGB(255, 255, 255, 255)),
                          textAlign: TextAlign.center,
                        ),
                        buildMainImageCard(appState.latestImage!),
                        const SizedBox(height: 16),
                        buildMetricsCard(appState),
                        const SizedBox(height: 16),
                        HistorySection(appState: appState, maxItems: 8),
                        const SizedBox(height: 16),
                        buildGraphSection(appState),
                        const SizedBox(height: 32)
                      ],
                    ),
          )
        )
      );
    }
}