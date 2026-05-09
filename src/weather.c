/* Weather-driven palette mode.
 *
 * Fetches the current condition for a postal code via wttr.in, a free
 * keyless weather service. Network access is delegated to `curl`,
 * which is part of the base install on every modern POSIX system —
 * keeps HaikalaC itself free of any C-level network dependency.
 *
 * The zip is sanitized before being interpolated into the popen
 * command (alphanumeric + hyphens only, ≤ 16 chars). The response is
 * a short human-readable string like "Partly cloudy" or "Light rain
 * shower"; classify() walks it and maps to an `hk_weather_cond`.
 *
 * Season comes from the system clock (Northern-hemisphere mapping —
 * good enough for a meditative terminal app; users in the southern
 * hemisphere can override with --season later if anyone asks).
 */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "haikala.h"

hk_season hk_season_now(void)
{
    time_t now = time(NULL);
    struct tm tmv;
    /* localtime_r is POSIX; fall back to localtime if unavailable. */
#if defined(_POSIX_C_SOURCE) || defined(_DEFAULT_SOURCE)
    localtime_r(&now, &tmv);
#else
    tmv = *localtime(&now);
#endif
    int m = tmv.tm_mon + 1; /* 1..12 */
    if (m == 12 || m <= 2) return HK_SEASON_WINTER;
    if (m <= 5)            return HK_SEASON_SPRING;
    if (m <= 8)            return HK_SEASON_SUMMER;
    return HK_SEASON_AUTUMN;
}

static bool valid_zip(const char *zip)
{
    if (!zip || !*zip) return false;
    size_t len = strlen(zip);
    if (len > 16) return false;
    for (size_t i = 0; i < len; ++i) {
        char c = zip[i];
        bool ok = (c >= '0' && c <= '9')
               || (c >= 'a' && c <= 'z')
               || (c >= 'A' && c <= 'Z')
               || c == '-';
        if (!ok) return false;
    }
    return true;
}

static hk_weather_cond classify(const char *text)
{
    if (!text) return HK_WEATHER_UNKNOWN;
    /* Lowercase a working copy for substring matching. */
    char low[256];
    size_t i = 0;
    for (; text[i] && i < sizeof(low) - 1; ++i) {
        unsigned char c = (unsigned char)text[i];
        low[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : (char)c;
    }
    low[i] = '\0';

    /* Order matters — check stronger conditions first. */
    if (strstr(low, "thunder") || strstr(low, "storm")) return HK_WEATHER_STORM;
    if (strstr(low, "snow") || strstr(low, "blizzard") ||
        strstr(low, "sleet")) return HK_WEATHER_SNOW;
    if (strstr(low, "rain") || strstr(low, "drizzle") ||
        strstr(low, "shower")) return HK_WEATHER_RAIN;
    if (strstr(low, "fog") || strstr(low, "mist") ||
        strstr(low, "haze") || strstr(low, "smoke")) return HK_WEATHER_FOG;
    if (strstr(low, "overcast") || strstr(low, "cloud")) return HK_WEATHER_CLOUDY;
    if (strstr(low, "clear") || strstr(low, "sunny") ||
        strstr(low, "fair")) return HK_WEATHER_CLEAR;
    return HK_WEATHER_UNKNOWN;
}

bool hk_weather_fetch(const char *zip, hk_weather *out)
{
    if (!out) return false;
    if (!valid_zip(zip)) return false;

    /* Fetch the human-readable condition only — `?format=%C`. Plain
     * HTTP avoids needing a TLS client; wttr.in supports both. The
     * `--max-time 5` keeps a slow link from stalling the app. */
    char cmd[256];
    int n = snprintf(cmd, sizeof(cmd),
                     "curl -fsS --max-time 5 'http://wttr.in/%s?format=%%C' "
                     "2>/dev/null",
                     zip);
    if (n < 0 || (size_t)n >= sizeof(cmd)) return false;

    FILE *p = popen(cmd, "r");
    if (!p) return false;

    char buf[512];
    if (!fgets(buf, sizeof(buf), p)) {
        pclose(p);
        return false;
    }
    int rc = pclose(p);
    if (rc == -1) return false;

    /* Trim trailing whitespace + newline. */
    size_t len = strlen(buf);
    while (len > 0) {
        unsigned char c = (unsigned char)buf[len - 1];
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            buf[--len] = '\0';
        } else {
            break;
        }
    }
    if (len == 0) return false;

    /* wttr.in error responses are HTML; skip anything that doesn't look
     * like a short, mostly-printable label. */
    if (buf[0] == '<' || strchr(buf, '<')) return false;
    if (len > 100) return false;

    snprintf(out->raw_text, sizeof(out->raw_text), "%s", buf);
    out->season    = hk_season_now();
    out->condition = classify(buf);
    return true;
}

const char *hk_weather_cond_name(hk_weather_cond c)
{
    switch (c) {
    case HK_WEATHER_CLEAR:   return "clear";
    case HK_WEATHER_CLOUDY:  return "cloudy";
    case HK_WEATHER_RAIN:    return "rain";
    case HK_WEATHER_SNOW:    return "snow";
    case HK_WEATHER_STORM:   return "storm";
    case HK_WEATHER_FOG:     return "fog";
    case HK_WEATHER_UNKNOWN:
    default:                 return "unknown";
    }
}

const char *hk_season_name(hk_season s)
{
    switch (s) {
    case HK_SEASON_SPRING: return "spring";
    case HK_SEASON_SUMMER: return "summer";
    case HK_SEASON_AUTUMN: return "autumn";
    case HK_SEASON_WINTER: return "winter";
    }
    return "?";
}
