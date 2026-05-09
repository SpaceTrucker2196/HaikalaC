/* Haiku table — a curated subset of public-domain classics. The Python
 * upstream ships 100; this initial port carries 10 to keep the data
 * file modest. Adding more is mechanical: extend the per-haiku token
 * arrays and the table at the bottom. */

#include <stdio.h>
#include <string.h>

#include "haikala.h"

/* ---- per-haiku token arrays ---------------------------------------- */

static const char *const tokens_old_pond[] = {
    "pond", "frog", "splash", "water", "stillness",
};
static const char *const tokens_withered_branch[] = {
    "crow", "branch", "evening", "stillness",
};
static const char *const tokens_summer_grass[] = {
    "grass", "warriors", "dream", "memory",
};
static const char *const tokens_first_snow_bridge[] = {
    "snow", "bridge", "stillness", "first",
};
static const char *const tokens_lightning_heron[] = {
    "lightning", "heron", "sea", "awakening",
};
static const char *const tokens_temple_bell[] = {
    "bell", "temple", "blossom", "echo",
};
static const char *const tokens_morning_glory_well[] = {
    "morning_glory", "well", "water", "longing",
};
static const char *const tokens_wild_sea_stars[] = {
    "sea", "star", "wave", "night", "vastness",
};
static const char *const tokens_first_dream[] = {
    "dream", "moon", "first", "joy",
};
static const char *const tokens_autumn_road[] = {
    "road", "evening", "crow", "loneliness",
};

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

const hk_haiku hk_haiku_table[] = {
    {
        .id = "old_pond",
        .line1 = "old pond—",
        .line2 = "a frog leaps in",
        .line3 = "the sound of water",
        .author = "Matsuo Bashō",
        .season = "spring",
        .tokens = tokens_old_pond,
        .n_tokens = COUNT(tokens_old_pond),
    },
    {
        .id = "withered_branch",
        .line1 = "on a withered branch",
        .line2 = "a crow has settled—",
        .line3 = "autumn nightfall",
        .author = "Matsuo Bashō",
        .season = "autumn",
        .tokens = tokens_withered_branch,
        .n_tokens = COUNT(tokens_withered_branch),
    },
    {
        .id = "summer_grass",
        .line1 = "summer grasses—",
        .line2 = "all that remains",
        .line3 = "of warriors' dreams",
        .author = "Matsuo Bashō",
        .season = "summer",
        .tokens = tokens_summer_grass,
        .n_tokens = COUNT(tokens_summer_grass),
    },
    {
        .id = "first_snow_bridge",
        .line1 = "first snow",
        .line2 = "falling on the half-finished",
        .line3 = "bridge",
        .author = "Matsuo Bashō",
        .season = "winter",
        .tokens = tokens_first_snow_bridge,
        .n_tokens = COUNT(tokens_first_snow_bridge),
    },
    {
        .id = "lightning_heron",
        .line1 = "a flash of lightning—",
        .line2 = "into the gloom",
        .line3 = "the cry of a heron",
        .author = "Matsuo Bashō",
        .season = "summer",
        .tokens = tokens_lightning_heron,
        .n_tokens = COUNT(tokens_lightning_heron),
    },
    {
        .id = "temple_bell",
        .line1 = "the temple bell stops—",
        .line2 = "but the sound keeps coming",
        .line3 = "out of the flowers",
        .author = "Matsuo Bashō",
        .season = "spring",
        .tokens = tokens_temple_bell,
        .n_tokens = COUNT(tokens_temple_bell),
    },
    {
        .id = "morning_glory_well",
        .line1 = "morning glory—",
        .line2 = "the well bucket entangled,",
        .line3 = "I ask for water",
        .author = "Fukuda Chiyo-ni",
        .season = "summer",
        .tokens = tokens_morning_glory_well,
        .n_tokens = COUNT(tokens_morning_glory_well),
    },
    {
        .id = "wild_sea_stars",
        .line1 = "wild seas—",
        .line2 = "spreading toward Sado",
        .line3 = "the river of stars",
        .author = "Matsuo Bashō",
        .season = "summer",
        .tokens = tokens_wild_sea_stars,
        .n_tokens = COUNT(tokens_wild_sea_stars),
    },
    {
        .id = "first_dream",
        .line1 = "what a moon—",
        .line2 = "the thief paused",
        .line3 = "to sing",
        .author = "Yosa Buson",
        .season = "autumn",
        .tokens = tokens_first_dream,
        .n_tokens = COUNT(tokens_first_dream),
    },
    {
        .id = "autumn_road",
        .line1 = "this road—",
        .line2 = "no one travels it,",
        .line3 = "this autumn evening",
        .author = "Matsuo Bashō",
        .season = "autumn",
        .tokens = tokens_autumn_road,
        .n_tokens = COUNT(tokens_autumn_road),
    },
};

const size_t hk_haiku_count = sizeof(hk_haiku_table) / sizeof(hk_haiku_table[0]);

const hk_haiku *hk_haiku_get(const char *id)
{
    if (id == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < hk_haiku_count; ++i) {
        if (strcmp(hk_haiku_table[i].id, id) == 0) {
            return &hk_haiku_table[i];
        }
    }
    return NULL;
}
