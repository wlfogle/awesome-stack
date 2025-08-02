import 'package:flutter/material.dart';

import 'package:lunasea/database/models/indexer.dart';
import 'package:lunasea/modules.dart';
import 'package:lunasea/modules/settings/routes/configuration/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_general/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_dashboard/pages/calendar_settings.dart';
import 'package:lunasea/modules/settings/routes/configuration_dashboard/pages/default_pages.dart';
import 'package:lunasea/modules/settings/routes/configuration_dashboard/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_drawer/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_external_modules/pages/add_module.dart';
import 'package:lunasea/modules/settings/routes/configuration_external_modules/pages/edit_module.dart';
import 'package:lunasea/modules/settings/routes/configuration_external_modules/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_lidarr/pages/connection_details.dart';
import 'package:lunasea/modules/settings/routes/configuration_lidarr/pages/default_pages.dart';
import 'package:lunasea/modules/settings/routes/configuration_lidarr/pages/headers.dart';
import 'package:lunasea/modules/settings/routes/configuration_lidarr/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_nzbget/pages/connection_details.dart';
import 'package:lunasea/modules/settings/routes/configuration_nzbget/pages/default_pages.dart';
import 'package:lunasea/modules/settings/routes/configuration_nzbget/pages/headers.dart';
import 'package:lunasea/modules/settings/routes/configuration_nzbget/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_quick_actions/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_radarr/pages/connection_details.dart';
import 'package:lunasea/modules/settings/routes/configuration_radarr/pages/default_options.dart';
import 'package:lunasea/modules/settings/routes/configuration_radarr/pages/default_pages.dart';
import 'package:lunasea/modules/settings/routes/configuration_radarr/pages/headers.dart';
import 'package:lunasea/modules/settings/routes/configuration_radarr/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_sabnzbd/pages/connection_details.dart';
import 'package:lunasea/modules/settings/routes/configuration_sabnzbd/pages/default_pages.dart';
import 'package:lunasea/modules/settings/routes/configuration_sabnzbd/pages/headers.dart';
import 'package:lunasea/modules/settings/routes/configuration_sabnzbd/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_search/pages/add_indexer.dart';
import 'package:lunasea/modules/settings/routes/configuration_search/pages/add_indexer_headers.dart';
import 'package:lunasea/modules/settings/routes/configuration_search/pages/edit_indexer.dart';
import 'package:lunasea/modules/settings/routes/configuration_search/pages/edit_indexer_headers.dart';
import 'package:lunasea/modules/settings/routes/configuration_search/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_sonarr/pages/connection_details.dart';
import 'package:lunasea/modules/settings/routes/configuration_sonarr/pages/default_options.dart';
import 'package:lunasea/modules/settings/routes/configuration_sonarr/pages/default_pages.dart';
import 'package:lunasea/modules/settings/routes/configuration_sonarr/pages/headers.dart';
import 'package:lunasea/modules/settings/routes/configuration_sonarr/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_tautulli/pages/connection_details.dart';
import 'package:lunasea/modules/settings/routes/configuration_tautulli/pages/default_pages.dart';
import 'package:lunasea/modules/settings/routes/configuration_tautulli/pages/headers.dart';
import 'package:lunasea/modules/settings/routes/configuration_tautulli/route.dart';
import 'package:lunasea/modules/settings/routes/configuration_wake_on_lan/route.dart';
import 'package:lunasea/modules/settings/routes/profiles/route.dart';
import 'package:lunasea/modules/settings/routes/settings/route.dart';
import 'package:lunasea/modules/settings/routes/system/route.dart';
import 'package:lunasea/modules/settings/routes/system_logs/pages/log_details.dart';
import 'package:lunasea/modules/settings/routes/system_logs/route.dart';
import 'package:lunasea/router/routes.dart';
import 'package:lunasea/types/log_type.dart';
import 'package:lunasea/vendor.dart';

enum SettingsRoutes with LunaRoutesMixin {
  HOME('/settings'),
  CONFIGURATION('configuration'),
  CONFIGURATION_GENERAL('general'),
  CONFIGURATION_DASHBOARD('dashboard'),
  CONFIGURATION_DASHBOARD_CALENDAR('calendar'),
  CONFIGURATION_DASHBOARD_DEFAULT_PAGES('default_pages'),
  CONFIGURATION_DRAWER('drawer'),
  CONFIGURATION_EXTERNAL_MODULES('external_modules'),
  CONFIGURATION_EXTERNAL_MODULES_ADD('add'),
  CONFIGURATION_EXTERNAL_MODULES_EDIT('edit/:id'),
  CONFIGURATION_LIDARR('lidarr'),
  CONFIGURATION_LIDARR_CONNECTION_DETAILS('connection_details'),
  CONFIGURATION_LIDARR_CONNECTION_DETAILS_HEADERS('headers'),
  CONFIGURATION_LIDARR_DEFAULT_PAGES('default_pages'),
  CONFIGURATION_NZBGET('nzbget'),
  CONFIGURATION_NZBGET_CONNECTION_DETAILS('connection_details'),
  CONFIGURATION_NZBGET_CONNECTION_DETAILS_HEADERS('headers'),
  CONFIGURATION_NZBGET_DEFAULT_PAGES('default_pages'),
  CONFIGURATION_QUICK_ACTIONS('quick_actions'),
  CONFIGURATION_RADARR('radarr'),
  CONFIGURATION_RADARR_CONNECTION_DETAILS('connection_details'),
  CONFIGURATION_RADARR_CONNECTION_DETAILS_HEADERS('headers'),
  CONFIGURATION_RADARR_DEFAULT_OPTIONS('default_options'),
  CONFIGURATION_RADARR_DEFAULT_PAGES('default_pages'),
  CONFIGURATION_SABNZBD('sabnzbd'),
  CONFIGURATION_SABNZBD_CONNECTION_DETAILS('connection_details'),
  CONFIGURATION_SABNZBD_CONNECTION_DETAILS_HEADERS('headers'),
  CONFIGURATION_SABNZBD_DEFAULT_PAGES('default_pages'),
  CONFIGURATION_SEARCH('search'),
  CONFIGURATION_SEARCH_ADD_INDEXER('add_indexer'),
  CONFIGURATION_SEARCH_ADD_INDEXER_HEADERS('headers'),
  CONFIGURATION_SEARCH_EDIT_INDEXER('edit_indexer/:id'),
  CONFIGURATION_SEARCH_EDIT_INDEXER_HEADERS('headers'),
  CONFIGURATION_SONARR('sonarr'),
  CONFIGURATION_SONARR_CONNECTION_DETAILS('connection_details'),
  CONFIGURATION_SONARR_CONNECTION_DETAILS_HEADERS('headers'),
  CONFIGURATION_SONARR_DEFAULT_OPTIONS('default_options'),
  CONFIGURATION_SONARR_DEFAULT_PAGES('default_pages'),
  CONFIGURATION_TAUTULLI('tautulli'),
  CONFIGURATION_TAUTULLI_CONNECTION_DETAILS('connection_details'),
  CONFIGURATION_TAUTULLI_CONNECTION_DETAILS_HEADERS('headers'),
  CONFIGURATION_TAUTULLI_DEFAULT_PAGES('default_pages'),
  CONFIGURATION_WAKE_ON_LAN('wake_on_lan'),
  PROFILES('profiles'),
  SYSTEM('system'),
  SYSTEM_LOGS('logs'),
  SYSTEM_LOGS_DETAILS('view/:type');

  @override
  final String path;

  const SettingsRoutes(this.path);

  @override
  LunaModule get module => LunaModule.SETTINGS;

  @override
  bool isModuleEnabled(BuildContext context) => true;

  @override
  GoRoute get routes {
    switch (this) {
      case SettingsRoutes.HOME:
        return route(widget: const SettingsRoute());
      case SettingsRoutes.CONFIGURATION:
        return route(widget: const ConfigurationRoute());
      case SettingsRoutes.CONFIGURATION_GENERAL:
        return route(widget: const ConfigurationGeneralRoute());
      case SettingsRoutes.CONFIGURATION_DASHBOARD:
        return route(widget: const ConfigurationDashboardRoute());
      case SettingsRoutes.CONFIGURATION_DASHBOARD_CALENDAR:
        return route(widget: const ConfigurationDashboardCalendarRoute());
      case SettingsRoutes.CONFIGURATION_DASHBOARD_DEFAULT_PAGES:
        return route(widget: const ConfigurationDashboardDefaultPagesRoute());
      case SettingsRoutes.CONFIGURATION_DRAWER:
        return route(widget: const ConfigurationDrawerRoute());
      case SettingsRoutes.CONFIGURATION_EXTERNAL_MODULES:
        return route(widget: const ConfigurationExternalModulesRoute());
      case SettingsRoutes.CONFIGURATION_EXTERNAL_MODULES_ADD:
        return route(widget: const ConfigurationExternalModulesAddRoute());
      case SettingsRoutes.CONFIGURATION_EXTERNAL_MODULES_EDIT:
        return route(builder: (_, state) {
          final moduleId = int.tryParse(state.pathParameters['id']!) ?? -1;
          return ConfigurationExternalModulesEditRoute(moduleId: moduleId);
        });
      case SettingsRoutes.CONFIGURATION_LIDARR:
        return route(widget: const ConfigurationLidarrRoute());
      case SettingsRoutes.CONFIGURATION_LIDARR_CONNECTION_DETAILS:
        return route(widget: const ConfigurationLidarrConnectionDetailsRoute());
      case SettingsRoutes.CONFIGURATION_LIDARR_CONNECTION_DETAILS_HEADERS:
        return route(
          widget: const ConfigurationLidarrConnectionDetailsHeadersRoute(),
        );
      case SettingsRoutes.CONFIGURATION_LIDARR_DEFAULT_PAGES:
        return route(widget: const ConfigurationLidarrDefaultPagesRoute());
      case SettingsRoutes.CONFIGURATION_NZBGET:
        return route(widget: const ConfigurationNZBGetRoute());
      case SettingsRoutes.CONFIGURATION_NZBGET_CONNECTION_DETAILS:
        return route(widget: const ConfigurationNZBGetConnectionDetailsRoute());
      case SettingsRoutes.CONFIGURATION_NZBGET_CONNECTION_DETAILS_HEADERS:
        return route(
          widget: const ConfigurationNZBGetConnectionDetailsHeadersRoute(),
        );
      case SettingsRoutes.CONFIGURATION_NZBGET_DEFAULT_PAGES:
        return route(widget: const ConfigurationNZBGetDefaultPagesRoute());
      case SettingsRoutes.CONFIGURATION_QUICK_ACTIONS:
        return route(widget: const ConfigurationQuickActionsRoute());
      case SettingsRoutes.CONFIGURATION_RADARR:
        return route(widget: const ConfigurationRadarrRoute());
      case SettingsRoutes.CONFIGURATION_RADARR_CONNECTION_DETAILS:
        return route(widget: const ConfigurationRadarrConnectionDetailsRoute());
      case SettingsRoutes.CONFIGURATION_RADARR_CONNECTION_DETAILS_HEADERS:
        return route(
          widget: const ConfigurationRadarrConnectionDetailsHeadersRoute(),
        );
      case SettingsRoutes.CONFIGURATION_RADARR_DEFAULT_OPTIONS:
        return route(widget: const ConfigurationRadarrDefaultOptionsRoute());
      case SettingsRoutes.CONFIGURATION_RADARR_DEFAULT_PAGES:
        return route(widget: const ConfigurationRadarrDefaultPagesRoute());
      case SettingsRoutes.CONFIGURATION_SABNZBD:
        return route(widget: const ConfigurationSABnzbdRoute());
      case SettingsRoutes.CONFIGURATION_SABNZBD_CONNECTION_DETAILS:
        return route(
          widget: const ConfigurationSABnzbdConnectionDetailsRoute(),
        );
      case SettingsRoutes.CONFIGURATION_SABNZBD_CONNECTION_DETAILS_HEADERS:
        return route(
          widget: const ConfigurationSABnzbdConnectionDetailsHeadersRoute(),
        );
      case SettingsRoutes.CONFIGURATION_SABNZBD_DEFAULT_PAGES:
        return route(widget: const ConfigurationSABnzbdDefaultPagesRoute());
      case SettingsRoutes.CONFIGURATION_SEARCH:
        return route(widget: const ConfigurationSearchRoute());
      case SettingsRoutes.CONFIGURATION_SEARCH_ADD_INDEXER:
        return route(widget: const ConfigurationSearchAddIndexerRoute());
      case SettingsRoutes.CONFIGURATION_SEARCH_ADD_INDEXER_HEADERS:
        return route(builder: (_, state) {
          final indexer = state.extra as LunaIndexer?;
          return ConfigurationSearchAddIndexerHeadersRoute(indexer: indexer);
        });
      case SettingsRoutes.CONFIGURATION_SEARCH_EDIT_INDEXER:
        return route(builder: (_, state) {
          final id = int.tryParse(state.pathParameters['id']!) ?? -1;
          return ConfigurationSearchEditIndexerRoute(id: id);
        });
      case SettingsRoutes.CONFIGURATION_SEARCH_EDIT_INDEXER_HEADERS:
        return route(builder: (_, state) {
          final id = int.tryParse(state.pathParameters['id']!) ?? -1;
          return ConfigurationSearchEditIndexerHeadersRoute(id: id);
        });
      case SettingsRoutes.CONFIGURATION_SONARR:
        return route(widget: const ConfigurationSonarrRoute());
      case SettingsRoutes.CONFIGURATION_SONARR_CONNECTION_DETAILS:
        return route(widget: const ConfigurationSonarrConnectionDetailsRoute());
      case SettingsRoutes.CONFIGURATION_SONARR_CONNECTION_DETAILS_HEADERS:
        return route(
          widget: const ConfigurationSonarrConnectionDetailsHeadersRoute(),
        );
      case SettingsRoutes.CONFIGURATION_SONARR_DEFAULT_OPTIONS:
        return route(widget: const ConfigurationSonarrDefaultOptionsRoute());
      case SettingsRoutes.CONFIGURATION_SONARR_DEFAULT_PAGES:
        return route(widget: const ConfigurationSonarrDefaultPagesRoute());
      case SettingsRoutes.CONFIGURATION_TAUTULLI:
        return route(widget: const ConfigurationTautulliRoute());
      case SettingsRoutes.CONFIGURATION_TAUTULLI_CONNECTION_DETAILS:
        return route(
          widget: const ConfigurationTautulliConnectionDetailsRoute(),
        );
      case SettingsRoutes.CONFIGURATION_TAUTULLI_CONNECTION_DETAILS_HEADERS:
        return route(
          widget: const ConfigurationTautulliConnectionDetailsHeadersRoute(),
        );
      case SettingsRoutes.CONFIGURATION_TAUTULLI_DEFAULT_PAGES:
        return route(widget: const ConfigurationTautulliDefaultPagesRoute());
      case SettingsRoutes.CONFIGURATION_WAKE_ON_LAN:
        return route(widget: const ConfigurationWakeOnLANRoute());
      case SettingsRoutes.PROFILES:
        return route(widget: const ProfilesRoute());
      case SettingsRoutes.SYSTEM:
        return route(widget: const SystemRoute());
      case SettingsRoutes.SYSTEM_LOGS:
        return route(widget: const SystemLogsRoute());
      case SettingsRoutes.SYSTEM_LOGS_DETAILS:
        return route(builder: (_, state) {
          final type = LunaLogType.fromKey(state.pathParameters['type']!);
          return SystemLogsDetailsRoute(type: type);
        });
    }
  }

  @override
  List<GoRoute> get subroutes {
    switch (this) {
      case SettingsRoutes.HOME:
        return [
          SettingsRoutes.CONFIGURATION.routes,
          SettingsRoutes.PROFILES.routes,
          SettingsRoutes.SYSTEM.routes,
        ];
      case SettingsRoutes.CONFIGURATION:
        return [
          SettingsRoutes.CONFIGURATION_GENERAL.routes,
          SettingsRoutes.CONFIGURATION_DASHBOARD.routes,
          SettingsRoutes.CONFIGURATION_DRAWER.routes,
          SettingsRoutes.CONFIGURATION_EXTERNAL_MODULES.routes,
          SettingsRoutes.CONFIGURATION_LIDARR.routes,
          SettingsRoutes.CONFIGURATION_NZBGET.routes,
          SettingsRoutes.CONFIGURATION_QUICK_ACTIONS.routes,
          SettingsRoutes.CONFIGURATION_RADARR.routes,
          SettingsRoutes.CONFIGURATION_SABNZBD.routes,
          SettingsRoutes.CONFIGURATION_SEARCH.routes,
          SettingsRoutes.CONFIGURATION_SONARR.routes,
          SettingsRoutes.CONFIGURATION_TAUTULLI.routes,
          SettingsRoutes.CONFIGURATION_WAKE_ON_LAN.routes,
        ];
      case SettingsRoutes.CONFIGURATION_DASHBOARD:
        return [
          SettingsRoutes.CONFIGURATION_DASHBOARD_CALENDAR.routes,
          SettingsRoutes.CONFIGURATION_DASHBOARD_DEFAULT_PAGES.routes,
        ];
      case SettingsRoutes.CONFIGURATION_LIDARR:
        return [
          SettingsRoutes.CONFIGURATION_LIDARR_CONNECTION_DETAILS.routes,
          SettingsRoutes.CONFIGURATION_LIDARR_DEFAULT_PAGES.routes,
        ];
      case SettingsRoutes.CONFIGURATION_LIDARR_CONNECTION_DETAILS:
        return [
          SettingsRoutes.CONFIGURATION_LIDARR_CONNECTION_DETAILS_HEADERS.routes,
        ];
      case SettingsRoutes.CONFIGURATION_NZBGET:
        return [
          SettingsRoutes.CONFIGURATION_NZBGET_CONNECTION_DETAILS.routes,
          SettingsRoutes.CONFIGURATION_NZBGET_DEFAULT_PAGES.routes,
        ];
      case SettingsRoutes.CONFIGURATION_NZBGET_CONNECTION_DETAILS:
        return [
          SettingsRoutes.CONFIGURATION_NZBGET_CONNECTION_DETAILS_HEADERS.routes,
        ];
      case SettingsRoutes.CONFIGURATION_RADARR:
        return [
          SettingsRoutes.CONFIGURATION_RADARR_CONNECTION_DETAILS.routes,
          SettingsRoutes.CONFIGURATION_RADARR_DEFAULT_OPTIONS.routes,
          SettingsRoutes.CONFIGURATION_RADARR_DEFAULT_PAGES.routes,
        ];
      case SettingsRoutes.CONFIGURATION_RADARR_CONNECTION_DETAILS:
        return [
          SettingsRoutes.CONFIGURATION_RADARR_CONNECTION_DETAILS_HEADERS.routes,
        ];
      case SettingsRoutes.CONFIGURATION_SABNZBD:
        return [
          SettingsRoutes.CONFIGURATION_SABNZBD_CONNECTION_DETAILS.routes,
          SettingsRoutes.CONFIGURATION_SABNZBD_DEFAULT_PAGES.routes,
        ];
      case SettingsRoutes.CONFIGURATION_SABNZBD_CONNECTION_DETAILS:
        return [
          SettingsRoutes
              .CONFIGURATION_SABNZBD_CONNECTION_DETAILS_HEADERS.routes,
        ];
      case SettingsRoutes.CONFIGURATION_SEARCH:
        return [
          SettingsRoutes.CONFIGURATION_SEARCH_ADD_INDEXER.routes,
          SettingsRoutes.CONFIGURATION_SEARCH_EDIT_INDEXER.routes,
        ];
      case SettingsRoutes.CONFIGURATION_SEARCH_ADD_INDEXER:
        return [
          SettingsRoutes.CONFIGURATION_SEARCH_ADD_INDEXER_HEADERS.routes,
        ];
      case SettingsRoutes.CONFIGURATION_SEARCH_EDIT_INDEXER:
        return [
          SettingsRoutes.CONFIGURATION_SEARCH_EDIT_INDEXER_HEADERS.routes,
        ];
      case SettingsRoutes.CONFIGURATION_SONARR:
        return [
          SettingsRoutes.CONFIGURATION_SONARR_CONNECTION_DETAILS.routes,
          SettingsRoutes.CONFIGURATION_SONARR_DEFAULT_OPTIONS.routes,
          SettingsRoutes.CONFIGURATION_SONARR_DEFAULT_PAGES.routes,
        ];
      case SettingsRoutes.CONFIGURATION_SONARR_CONNECTION_DETAILS:
        return [
          SettingsRoutes.CONFIGURATION_SONARR_CONNECTION_DETAILS_HEADERS.routes,
        ];
      case SettingsRoutes.CONFIGURATION_TAUTULLI:
        return [
          SettingsRoutes.CONFIGURATION_TAUTULLI_CONNECTION_DETAILS.routes,
          SettingsRoutes.CONFIGURATION_TAUTULLI_DEFAULT_PAGES.routes,
        ];
      case SettingsRoutes.CONFIGURATION_TAUTULLI_CONNECTION_DETAILS:
        return [
          SettingsRoutes
              .CONFIGURATION_TAUTULLI_CONNECTION_DETAILS_HEADERS.routes,
        ];
      case SettingsRoutes.CONFIGURATION_EXTERNAL_MODULES:
        return [
          SettingsRoutes.CONFIGURATION_EXTERNAL_MODULES_ADD.routes,
          SettingsRoutes.CONFIGURATION_EXTERNAL_MODULES_EDIT.routes,
        ];
      case SettingsRoutes.SYSTEM:
        return [
          SettingsRoutes.SYSTEM_LOGS.routes,
        ];
      case SettingsRoutes.SYSTEM_LOGS:
        return [
          SettingsRoutes.SYSTEM_LOGS_DETAILS.routes,
        ];
      default:
        return const <GoRoute>[];
    }
  }
}
