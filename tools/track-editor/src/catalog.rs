//! Object IDs registered by `GeoFuncTable` in `Sato/GeographyObjMgr.cpp`.
//!
//! `placeable` describes normal course-object records, not whether an object can
//! exist in a scene. It is true for factories which consume an `SObject` and
//! false for `NewS` factories: the latter are generated subobjects or obtain
//! their placement elsewhere (for example, the start line and Lakitu).

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ObjectInfo {
    pub id: u16,
    pub name: &'static str,
    pub class_name: &'static str,
    pub category: &'static str,
    pub placeable: bool,
}

const fn entry(
    id: u16,
    name: &'static str,
    class_name: &'static str,
    category: &'static str,
    placeable: bool,
) -> ObjectInfo {
    ObjectInfo {
        id,
        name,
        class_name,
        category,
        placeable,
    }
}

const ITEM: &str = "Items";
const TRACK: &str = "Track Feature";
const SCENERY: &str = "Scenery";
const HAZARD: &str = "Hazard";
const TRAFFIC: &str = "Traffic";
const EFFECT: &str = "Effects";
const GENERATOR: &str = "Generator";
const INTERNAL: &str = "Internal";
const GAMEPLAY: &str = "Gameplay";
const WATER: &str = "Water";
const BATTLE: &str = "Battle";
const AWARD: &str = "Awards";
const TEST: &str = "Test";
const AUDIO: &str = "Audio";

#[rustfmt::skip]
static OBJECTS: [ObjectInfo; 160] = [
    // GeoFuncTbl.
    entry(0x0001, "Item Box", "GeoItemBox", ITEM, true),
    entry(0x000c, "Double Item Box (Internal Variant)", "GeoDoubleItemBox", INTERNAL, false),
    entry(0x000a, "Path-following Item Box", "GeoF_ItemBox", ITEM, true),
    entry(0x0009, "Start Line (Automatic)", "GeoStartLine", INTERNAL, false),
    entry(0x0e75, "Mario Circuit Tree", "GeoMarioTree1", SCENERY, true),
    entry(0x0e76, "Mario Circuit Mushroom", "GeoMarioKinoko1", SCENERY, true),
    entry(0x0e77, "Mario Circuit Flower", "GeoMarioFlower1", SCENERY, true),
    entry(0x125d, "Thwomp", "TMapObjDossun", HAZARD, true),
    entry(0x0fa1, "Traffic Car", "GeoNormCar", TRAFFIC, true),
    entry(0x1003, "Traffic Car Shadow", "GeoCarShadow", INTERNAL, false),
    entry(0x1001, "Bob-omb Car Shadow", "GeoBombShadow", INTERNAL, false),
    entry(0x1002, "Mushroom Car Shadow", "GeoKinoShadow", INTERNAL, false),
    entry(0x1000, "Wiggler Head Shadow", "GeoHanaShadow", INTERNAL, false),
    entry(0x0fff, "Wiggler Body Shadow", "GeoHanaBodyShadow", INTERNAL, false),
    entry(0x0fa3, "Traffic Truck", "GeoTruck", TRAFFIC, true),
    entry(0x0fa2, "Traffic Bus", "GeoBus", TRAFFIC, true),
    entry(0x0fa5, "Bob-omb Car", "GeoBombCar", TRAFFIC, true),
    entry(0x0fa6, "Mushroom Car", "GeoKinoCar", TRAFFIC, true),
    entry(0x0fa7, "Goomba Car", "GeoKuriCar", TRAFFIC, true),
    entry(0x0fa8, "Item Box Car", "GeoItemCar", TRAFFIC, true),
    entry(0x0fa9, "Wiggler Head Car", "GeoHanaCar", TRAFFIC, true),
    entry(0x0faa, "Wiggler Body Segment", "GeoHanaBody", INTERNAL, false),
    entry(0x00ff, "Lakitu", "TJugem", INTERNAL, false),
    entry(0x0101, "Lakitu Rod", "TJugemVoidRod", INTERNAL, false),
    entry(0x0100, "Lakitu Headset", "TJugemHeadHear", INTERNAL, false),
    entry(0x0104, "Lakitu Sign Board", "TJugemRodBoard", INTERNAL, false),
    entry(0x0105, "Lakitu Sign Board 2", "TJugemRodBoard2", INTERNAL, false),
    entry(0x0106, "Lakitu Reverse Sign Board", "TJugemRodBoardRev", INTERNAL, false),
    entry(0x0107, "Lakitu Traffic Light", "TJugemRodSignal", INTERNAL, false),
    entry(0x0108, "Lakitu PukuPuku Sign", "TJugemRodPukuPuku", INTERNAL, false),
    entry(0x0102, "Lakitu Flag", "TJugemFlag", INTERNAL, false),
    entry(0x0109, "Lakitu Head Lamp", "TJugemHeadLamp", INTERNAL, false),
    entry(0x0fa4, "Traffic Light", "GeoSignal", TRACK, true),
    entry(0x1195, "Cannon", "GeoCannon", TRACK, true),
    entry(0x0e78, "Chain Chomp", "TMapObjWanwan", HAZARD, true),
    entry(0x0e79, "Chain Chomp Chain", "TMapObjWanwanChain", INTERNAL, false),
    entry(0x0e7a, "Chain Chomp Post", "TMapObjWanwanPile", INTERNAL, false),
    entry(0x0e7e, "Airship", "TMapObjSkyShip", SCENERY, true),
    entry(0x1197, "DK Boulder", "TMapObjDonkyRock", HAZARD, true),
    entry(0x1198, "DK Tree", "TMapObjDonkyTree", SCENERY, true),
    entry(0x1199, "Static Map Object", "TMapObjNoMove", SCENERY, true),
    entry(0x11a0, "DK Cloud", "GeoDkCloud", SCENERY, true),
    entry(0x0da4, "Water Surface", "GeoWaterUp", INTERNAL, false),
    entry(0x0da5, "Water Surface 2", "GeoWaterUp2", INTERNAL, false),
    entry(0x0d49, "Animated Water", "GeoWater", WATER, true),
    entry(0x1069, "Fire Bar", "TMapObjFireBar", HAZARD, true),
    entry(0x106d, "Vision", "GeoVision", EFFECT, true),
    entry(0x0d4a, "Poihana Flower", "TMapObjPoihana", HAZARD, true),
    entry(0x106e, "Wall", "TMapObjWall", TRACK, true),
    entry(0x119b, "Butterfly", "TSMAButterfly", INTERNAL, false),
    entry(0x119d, "DK Bird", "TSMADonkyBird", INTERNAL, false),
    entry(0x0d4c, "Seagull", "TSMAKamome", INTERNAL, false),
    entry(0x119e, "Geyser", "TMapObjGeyser", HAZARD, true),
    entry(0x119f, "Geyser Sensor", "TMapObjGeyserSearch", GAMEPLAY, true),
    entry(0x0e0f, "Rolling Platform", "TMapObjRollTable", TRACK, true),
    entry(0x0e0d, "Test String Barrel", "TTestStrBarrel", INTERNAL, false),
    entry(0x0e0b, "Test String Item", "TTestStrItem", INTERNAL, false),
    entry(0x0d4d, "Peach Beach Tree", "TMapObjPeachTree", SCENERY, true),
    entry(0x13ed, "Snow Rock", "TMapObjSnowRock", HAZARD, true),
    entry(0x0e0c, "Test Rolling Object", "TTestRollObj", TEST, true),
    entry(0x0d4e, "Fountain", "TMapObjFountain", SCENERY, true),
    entry(0x13ee, "Shy Guy", "TMapObjHeyho", SCENERY, true),
    entry(0x13ef, "Shy Guy Subobject", "TMapObjSubHeyho", INTERNAL, false),
    entry(0x1389, "Pokey", "TMapObjSanbo", HAZARD, true),
    entry(0x106f, "Waluigi Arrow", "TMapObjWlArrow", TRACK, true),
    entry(0x13f0, "Snowman", "TMapObjSnowMan", SCENERY, true),
    entry(0x13f2, "Ice Block", "TMapObjIceBlock", HAZARD, true),
    entry(0x1070, "Static Map Object", "TMapObjNoMove", SCENERY, true),
    entry(0x13f3, "Static Map Object", "TMapObjNoMove", SCENERY, true),
    entry(0x0e7f, "Goomba", "GeoKuribo", HAZARD, true),
    entry(0x0011, "Sun", "TMapObjSun", SCENERY, true),
    entry(0x125e, "Bubble", "TMapObjBubble", EFFECT, true),
    entry(0x138a, "Desert Sun", "TMapObjDesun", SCENERY, true),
    entry(0x138b, "Sand Pillar", "TMapObjSandPillar", HAZARD, true),
    entry(0x0ce5, "Ferris Wheel", "TMapObjFerriswheel", SCENERY, true),
    entry(0x0dae, "Moving Item Box (Limited)", "TMapObjMoveItemBoxLimit", ITEM, true),
    entry(0x26b4, "Shine Sprite", "GeoShine", INTERNAL, false),
    entry(0x26ae, "Battle Balloon", "TBalloon", INTERNAL, false),
    entry(0x0ce6, "Roller Coaster Front Car", "TMapObjJetCoasterHead", SCENERY, true),
    entry(0x0ce7, "Roller Coaster Car", "TMapObjJetCoasterBody", INTERNAL, false),
    entry(0x0daf, "Pool", "TMapObjPool", SCENERY, true),
    entry(0x138c, "Desert Balloon", "TMapObjDeBalloon", SCENERY, true),
    entry(0x125f, "Bowser Castle Flame", "TMapObjKpFire", HAZARD, true),
    entry(0x138d, "Pokey Head", "TMapObjSanboHead", INTERNAL, false),
    entry(0x138e, "Pokey Body Segment", "TMapObjSanboBody", INTERNAL, false),
    entry(0x0db0, "Rolling Platform", "TMapObjRollTable", TRACK, true),
    entry(0x26b2, "Moving Item Box", "TMapObjMoveItemBox", ITEM, true),
    entry(0x11a1, "Nossie", "TMapObjNossie", SCENERY, true),
    entry(0x0d4f, "Mare A", "TMapObjMareA", SCENERY, true),
    entry(0x0d50, "Mare B", "TMapObjMareB", SCENERY, true),
    entry(0x0d51, "Mare C", "TMapObjMareC", SCENERY, true),
    entry(0x0d52, "Monte A", "TMapObjMonteA", SCENERY, true),
    entry(0x0d53, "Monte B", "TMapObjMonteB", SCENERY, true),
    entry(0x0d54, "Monte C", "TMapObjMonteC", SCENERY, true),
    entry(0x0d58, "Monte D", "TMapObjMonteD", SCENERY, true),
    entry(0x0d59, "Monte E", "TMapObjMonteE", SCENERY, true),
    entry(0x0d5a, "Mare W A", "TMapObjMareW_A", SCENERY, true),
    entry(0x0d5b, "Mare W B", "TMapObjMareW_B", SCENERY, true),
    entry(0x0d5c, "Mare W C", "TMapObjMareW_C", SCENERY, true),
    entry(0x0d5d, "Mare M A", "TMapObjMareM_A", SCENERY, true),
    entry(0x1327, "Meteor", "TMapObjMeteor", EFFECT, true),
    entry(0x0d55, "Ukulele Monte", "TMapObjUkleleMonte", SCENERY, true),
    entry(0x0d56, "Dancing Monte A", "TMapObjDanceMonteA", SCENERY, true),
    entry(0x0d57, "Dancing Monte B", "TMapObjDanceMonteB", SCENERY, true),
    entry(0x0d66, "Award Item Box", "TMapObjAwardItemBox", AWARD, true),
    entry(0x0d67, "Mushroom Cup Trophy", "TMapObjAwardCupKinoko", INTERNAL, false),
    entry(0x0d68, "Flower Cup Trophy", "TMapObjAwardCupFlower", INTERNAL, false),
    entry(0x0d69, "Star Cup Trophy", "TMapObjAwardCupStar", INTERNAL, false),
    entry(0x0d6a, "Special Cup Trophy", "TMapObjAwardCupSpecial", INTERNAL, false),
    entry(0x0d6b, "Mirror Cup Trophy", "TMapObjAwardCupReverse", INTERNAL, false),
    entry(0x0d6c, "All Cup Tour Trophy", "TMapObjAwardCupAllTour", INTERNAL, false),
    entry(0x138f, "Antlion", "TMapObjAntLion", HAZARD, true),
    entry(0x0e80, "Piranha Plant", "TMapObjPakkun", HAZARD, true),
    entry(0x0d7c, "Toadsworth", "GeoKinojii", SCENERY, true),
    entry(0x11a4, "Dino Jungle Tree", "TMapObjDinoTree", SCENERY, true),
    entry(0x0d7d, "Demo Fountain", "TMapObjDemoFountain", SCENERY, true),
    entry(0x1261, "Bowser Castle Gear", "TMapObjKpGear", HAZARD, true),
    entry(0x1262, "Bowser Castle Fire Bar", "TMapObjKpFireBar", HAZARD, true),
    entry(0x11a5, "Swimming Nossie", "TMapObjSwimNossie", SCENERY, true),
    entry(0x11a6, "Pteranodon", "TMapObjPtera", SCENERY, true),
    entry(0x0d7f, "Peach Beach Tree 2", "TMapObjPeachTree2", SCENERY, true),
    entry(0x0ed9, "Yoshi Helicopter", "TMapObjYoshiHeli", SCENERY, true),
    entry(0x1392, "Desert Tree", "TMapObjDesertTree", SCENERY, true),
    entry(0x1072, "Waluigi Stadium Searchlight", "TMapObjWluSearchLight", EFFECT, true),
    entry(0x0db1, "Ventilator", "TMapObjVentilator", SCENERY, true),
    entry(0x26b5, "Shine Arrow", "TShineArrow", INTERNAL, false),
    entry(0x1329, "UFO", "TMapObjUFO", SCENERY, true),
    entry(0x0e82, "Mushroom Balloon", "TMapObjMashBalloon", SCENERY, true),
    entry(0x0ce8, "Path-following Yoshi Helicopter", "TMapObjPathYoshiHeli", SCENERY, true),
    entry(0x106a, "Fireball", "TMapObjFireBall", INTERNAL, false),
    entry(0x1263, "Bowser Castle Fire Bar Fireball", "TMapObjKpFireBarBall", INTERNAL, false),
    entry(0x1260, "Bowser Castle Fireball", "TMapObjKpFireBall", INTERNAL, false),

    // GeoPrevDrawFuncTbl: layers generated by GeoWater.
    entry(0x0da2, "Lower Water Layer", "GeoWaterDown", INTERNAL, false),
    entry(0x0da3, "Middle Water Layer", "GeoWaterMiddleD", INTERNAL, false),

    // GenGeoFuncTbl. Generators taking SObject are course-placeable; NewS
    // entries are internal helpers/subobjects.
    entry(0x1196, "DK Boulder Generator", "TMapObjDonkyRockGen", GENERATOR, true),
    entry(0x106b, "Fire Circle", "TMapObjFireCircle", HAZARD, true),
    entry(0x000e, "Item Generator", "GeoItemGen", BATTLE, true),
    entry(0x119a, "Butterfly Generator", "TSMAButterflyGen", GENERATOR, true),
    entry(0x119c, "DK Bird Generator", "TSMADonkyBirdGen", GENERATOR, true),
    entry(0x0d4b, "Bird Flock Manager", "TSMABoidBirdManager", GENERATOR, true),
    entry(0x0e0e, "Test String", "TTestString", TEST, true),
    entry(0x26ad, "Balloon String", "TBalloonString", INTERNAL, false),
    entry(0x0010, "Lens Flare", "TLensFlare", EFFECT, true),
    entry(0x13f1, "Distant Snowfall", "TDistSnow", EFFECT, true),
    entry(0x26af, "Balloon Manager", "TBalloonManager", INTERNAL, false),
    entry(0x13f4, "Air Jet", "GeoAirJet", EFFECT, true),
    entry(0x11a2, "Nossie Head Collision", "TMapObjNossieColHead", INTERNAL, false),
    entry(0x11a3, "Nossie Front Foot Collision", "TMapObjNossieColFootFront", INTERNAL, false),
    entry(0x11a7, "Nossie Rear Foot Collision", "TMapObjNossieColFootRear", INTERNAL, false),
    entry(0x11a8, "Nossie Body Collision", "TMapObjNossieColBody", INTERNAL, false),
    entry(0x0d7a, "Fireworks", "TMapObjHanabi", EFFECT, true),
    entry(0x1071, "Water Splash", "GeoSplash", EFFECT, true),
    entry(0x0d7e, "Tide Sensor", "GeoTideSensor", WATER, true),
    entry(0x1390, "Puller", "GeoPuller", GAMEPLAY, true),
    entry(0x1391, "Shimmer Effect", "GeoShimmer", EFFECT, true),
    entry(0x0e81, "Piranha Plant Head Collision", "TMapObjPakkunColHead", INTERNAL, false),
    entry(0x0013, "Sound Trigger", "GeoSoundGen", AUDIO, true),
    entry(0x26b3, "Rabbit Marker", "GeoRabbitMark", BATTLE, true),

    // BlockGeoFuncTbl and SpecialGeoFuncTbl.
    entry(0x0003, "Narrow Jump", "GeoJumpNarrow", TRACK, true),
    entry(0x0007, "Suspension Bridge", "StringBridge", TRACK, true),
];

/// Returns every registered object ID, in factory-table order.
/// Model files (lower-case base names, no extension) each object class loads,
/// from `getBmdFileName()` overrides in the decompilation. Variants follow
/// the game's own fallback order; the first one found in loaded archives wins.
#[rustfmt::skip]
static CLASS_MODELS: &[(&str, &[&str])] = &[
    ("GeoBombCar", &["car_bomb1"]),
    ("GeoBombShadow", &["car_bomb1shadow"]),
    ("GeoBus", &["car_bus1"]),
    ("GeoCannon", &["cannon1"]),
    ("GeoCarShadow", &["car_public1shadow"]),
    ("GeoDkCloud", &["donkycloud"]),
    ("GeoDoubleItemBox", &["itembox_double", "itemboxw_lan", "minigameitemboxw_bomb", "itembox_double_4p"]),
    ("GeoHanaBody", &["car_hana1body"]),
    ("GeoHanaCar", &["car_hana1"]),
    ("GeoHanaShadow", &["car_hana1shadow"]),
    ("GeoItemBox", &["itembox_normal", "itembox_lan"]),
    ("GeoItemCar", &["car_item1"]),
    ("GeoJumpNarrow", &["njump"]),
    ("GeoKinoCar", &["car_kinoko1"]),
    ("GeoKinoShadow", &["car_kinoko1shadow"]),
    ("GeoKinojii", &["kinojii"]),
    ("GeoKuriCar", &["car_kuri1"]),
    ("GeoKuribo", &["kuribo1"]),
    ("GeoMarioFlower1", &["marioflower1"]),
    ("GeoMarioKinoko1", &["mariokinoko1"]),
    ("GeoMarioTree1", &["mariotree1"]),
    ("GeoNormCar", &["car_public1"]),
    ("GeoRFlower", &["rflower"]),
    ("GeoShine", &["minigame_shine"]),
    ("GeoSignalSupervisor", &["signal1"]),
    ("GeoStartLine", &["startline"]),
    ("GeoTree", &["ki_00"]),
    ("GeoTruck", &["car_truck1"]),
    ("GeoVision", &["wl_screen1"]),
    ("GeoWater", &["sea1_spc"]),
    ("GeoWaterDown", &["sea5_sand"]),
    ("GeoWaterMiddleD", &["sea4_nami"]),
    ("GeoWaterUp", &["sea3_dark"]),
    ("GeoWaterUp2", &["sea2_tex"]),
    ("ItemFireBall", &["item_koura_green"]),
    ("ItemFlyTurtle", &["item_koura_yellow"]),
    ("ItemGTurtle", &["item_koura_green"]),
    ("ItemGTurtleBig", &["item_koura_koopa"]),
    ("ItemGoldenKinoko", &["item_kinoko_gold"]),
    ("ItemHeart", &["item_heart"]),
    ("ItemKinoko", &["item_kinoko"]),
    ("ItemRTurtle", &["item_koura_red"]),
    ("ItemStar", &["item_star"]),
    ("ItemThunder", &["item_thunder"]),
    ("ItemYoshiEgg", &["item_egg"]),
    ("J3DEfctIceRock", &["icerock"]),
    ("J3DEfctMudSplashDown", &["mizubashira_mud_a"]),
    ("J3DEfctSplashDown", &["mizubashira"]),
    ("StringBridge", &["hbmaruta"]),
    ("TBalloon", &["minigame_balloon"]),
    ("TJugem", &["jg_model", "jg_modell"]),
    ("TJugemFlag", &["jg_flag"]),
    ("TJugemHeadHear", &["jg_hair_model", "jg_hair_modell"]),
    ("TJugemHeadLamp", &["jg_patlamp"]),
    ("TJugemRodBoard", &["jg_board01"]),
    ("TJugemRodBoard2", &["boardfinallap"]),
    ("TJugemRodBoardRev", &["boardreverse"]),
    ("TJugemRodPukuPuku", &["puku_model"]),
    ("TJugemRodSignal", &["jg_signal"]),
    ("TJugemVoidRod", &["rod_model"]),
    ("TMapObjAntLion", &["antlion"]),
    ("TMapObjAwardCupAllTour", &["awardallcuptour"]),
    ("TMapObjAwardCupFlower", &["awardflowercup"]),
    ("TMapObjAwardCupKinoko", &["awardkinokocup"]),
    ("TMapObjAwardCupReverse", &["awardreversecup"]),
    ("TMapObjAwardCupSpecial", &["awardspecialcup"]),
    ("TMapObjAwardCupStar", &["awardstarcup"]),
    ("TMapObjDanceMonteA", &["monf_a"]),
    ("TMapObjDanceMonteB", &["monf_b"]),
    ("TMapObjDeBalloon", &["deballoon1"]),
    ("TMapObjDemoFountain", &["dpeachfountain"]),
    ("TMapObjDesertTree", &["deserttree1"]),
    ("TMapObjDesun", &["desun1"]),
    ("TMapObjDinoTree", &["dinotree1"]),
    ("TMapObjDonkyTree", &["donkytree1"]),
    ("TMapObjFireBar", &["firebar1"]),
    ("TMapObjFountain", &["peachfountain"]),
    ("TMapObjIceBlock", &["iceblock1"]),
    ("TMapObjKpFireBar", &["kpfirebar1"]),
    ("TMapObjKpGear", &["kpgear1"]),
    ("TMapObjMareA", &["marel_a"]),
    ("TMapObjMareB", &["marel_b"]),
    ("TMapObjMareC", &["marel_c"]),
    ("TMapObjMareW_A", &["marew_a"]),
    ("TMapObjMareW_B", &["marew_b"]),
    ("TMapObjMareW_C", &["marew_c"]),
    ("TMapObjMeteor", &["geostar"]),
    ("TMapObjMonteA", &["monl_a"]),
    ("TMapObjMonteB", &["monl_b"]),
    ("TMapObjMonteC", &["monl_c"]),
    ("TMapObjMonteD", &["monl_d"]),
    ("TMapObjMonteE", &["monl_e"]),
    ("TMapObjPeachTree", &["peachtree1"]),
    ("TMapObjPeachTree2", &["peachtree2"]),
    ("TMapObjRollTable", &["testwall1"]),
    ("TMapObjSnowMan", &["snowman1"]),
    ("TMapObjSun", &["sun"]),
    ("TMapObjVentilator", &["fan1"]),
    ("TMapObjWall", &["wl_wall1"]),
    ("TMapObjWanwan", &["wanwan1"]),
    ("TMapObjWanwanChain", &["wanwan_chn"]),
    ("TMapObjWanwanPile", &["wanwan1pile"]),
    ("TMapObjWlArrow", &["wlarrow1"]),
    ("TMapObjWluSearchLight", &["wa_search1"]),
    ("TSMAButterfly", &["butterfly1"]),
    ("TTestRollObj", &["testpaipo1"]),
];

/// `TMapObjNoMove` chooses its model by object ID (`sObjList` in
/// `Shiraiwa/MapObjNoMove.cpp`).
#[rustfmt::skip]
static STATIC_OBJECT_MODELS: &[(u16, &str)] = &[
    (0x1199, "donkywood"),
    (0x1070, "wl_dokan1"),
    (0x13f3, "lights1"),
    (0x138c, "deballoon1"),
];

/// Classes without a `getBmdFileName()` override in the decompilation yet,
/// matched to the model their course archives ship. Inferred from archive
/// contents rather than code.
#[rustfmt::skip]
static INFERRED_CLASS_MODELS: &[(&str, &[&str])] = &[
    ("GeoF_ItemBox", &["itembox_normal"]),
    ("GeoSignal", &["signal1"]),
    ("TMapObjAwardItemBox", &["itembox_normal"]),
    ("TMapObjBubble", &["bubble1"]),
    ("TMapObjDonkyRockGen", &["donkyrock1"]),
    ("TMapObjDossun", &["dossun1"]),
    ("TMapObjFerriswheel", &["babykanran"]),
    ("TMapObjGeyser", &["geyser1"]),
    ("TMapObjHeyho", &["heyho1"]),
    ("TMapObjJetCoasterBody", &["babyjet_body"]),
    ("TMapObjJetCoasterHead", &["babyjet_head"]),
    ("TMapObjKpFire", &["kpfire1"]),
    ("TMapObjMashBalloon", &["mash_balloon"]),
    ("TMapObjMoveItemBox", &["itembox_normal"]),
    ("TMapObjMoveItemBoxLimit", &["itembox_normal"]),
    ("TMapObjNossie", &["nossie"]),
    ("TMapObjPakkun", &["pakkun"]),
    ("TMapObjPathYoshiHeli", &["yoshihelib"]),
    ("TMapObjPoihana", &["poihana1"]),
    ("TMapObjPool", &["pool"]),
    ("TMapObjPtera", &["ptera"]),
    ("TMapObjSanbo", &["sanbo1"]),
    ("TMapObjSandPillar", &["tornado"]),
    ("TMapObjSkyShip", &["skyship1"]),
    ("TMapObjSnowRock", &["snowrock1"]),
    ("TMapObjSwimNossie", &["swimnossie"]),
    ("TMapObjUkleleMonte", &["uklele_monte"]),
    ("TMapObjYoshiHeli", &["yoshiheli"]),
    ("TSMABoidBirdManager", &["kamome1"]),
    ("TSMAButterflyGen", &["butterfly1"]),
    ("TSMADonkyBirdGen", &["bird1"]),
    ("TTestString", &["testhimo1"]),
];

/// Extra models an object spawns as sub-objects at runtime (for example
/// `GeoWater` creates `GeoWaterUp`/`Up2`/`MiddleD`/`Down` in its constructor).
pub fn companion_models(id: u16) -> &'static [&'static str] {
    match id {
        0x0d49 => &["sea5_sand", "sea4_nami", "sea3_dark", "sea2_tex"],
        _ => &[],
    }
}

/// Height the editor draws an object's models at, when the game moves them
/// away from the placed position. `GeoWater` oscillates between its low
/// (`params[6]`, default 450) and high (`params[7]`, default 800) tide.
pub fn display_height(id: u16, params: &[i16; 8], placed: f32) -> f32 {
    match id {
        0x0d49 => {
            let low = if params[6] != 0 { f32::from(params[6]) } else { 450.0 };
            let high = if params[7] != 0 { f32::from(params[7]) } else { 800.0 };
            (low + high) * 0.5
        }
        _ => placed,
    }
}

/// Candidate model base names for an object ID, most specific first.
pub fn model_names(id: u16) -> Vec<&'static str> {
    let mut names = Vec::new();
    if let Some((_, name)) = STATIC_OBJECT_MODELS.iter().find(|(object, _)| *object == id) {
        names.push(*name);
    }
    if let Some(info) = object_info(id) {
        for table in [CLASS_MODELS, INFERRED_CLASS_MODELS] {
            if let Some((_, models)) = table
                .iter()
                .find(|(class_name, _)| *class_name == info.class_name)
            {
                names.extend(models.iter().copied());
            }
        }
    }
    names
}

pub const fn objects() -> &'static [ObjectInfo] {
    &OBJECTS
}

pub fn object_info(id: u16) -> Option<&'static ObjectInfo> {
    OBJECTS.iter().find(|info| info.id == id)
}

pub fn object_name(id: u16) -> Option<&'static str> {
    object_info(id).map(|info| info.name)
}

const P1: &str = "Parameter 1";
const P2: &str = "Parameter 2";
const P3: &str = "Parameter 3";
const P4: &str = "Parameter 4";
const P5: &str = "Parameter 5";
const P6: &str = "Parameter 6";
const P7: &str = "Parameter 7";
const P8: &str = "Parameter 8";

const UNKNOWN_PARAMETERS: [&str; 8] = [P1, P2, P3, P4, P5, P6, P7, P8];

/// Labels for the eight signed 16-bit course-object parameters.
///
/// Slots left as `Parameter N` have no verified meaning here. In particular,
/// that does not imply that the game ignores those slots.
pub const fn parameter_labels(id: u16) -> [&'static str; 8] {
    match id {
        0x0001 | 0x000c => [
            "Height offset",
            "Box type (0 = random, 1 = single, 3 = double)",
            "Item shuffle option flag",
            P4,
            P5,
            P6,
            P7,
            P8,
        ],
        0x000a => [
            "Height offset above path",
            "Movement speed preset (0..2)",
            "Item shuffle option flag",
            P4,
            P5,
            P6,
            P7,
            P8,
        ],
        0x0dae | 0x26b2 => [
            "Height offset",
            P2,
            "Item shuffle option flag",
            P4,
            P5,
            P6,
            P7,
            P8,
        ],
        0x0fa1 | 0x0fa2 | 0x0fa3 | 0x0fa5 | 0x0fa6 | 0x0fa7 | 0x0fa8 => [
            "Maximum speed (0 = default)",
            "Zigzag amount (0..100)",
            P3,
            P4,
            "Target point radius (0 = default)",
            "Steering strength (0 = default)",
            P7,
            P8,
        ],
        0x0fa9 => [
            "Maximum speed (0 = default)",
            "Zigzag amount (0..100)",
            "Body segment count",
            P4,
            "Target point radius (0 = default)",
            "Steering strength (0 = default)",
            P7,
            P8,
        ],
        0x1195 => [
            "Cannon ID",
            "Launch angle (degrees from forward)",
            "Hidden flag",
            "Cannon feedback/effect type (0-2)",
            P5,
            P6,
            P7,
            P8,
        ],
        0x1069 | 0x1262 => [
            P1,
            "Rotation speed (value x 0.001; 0 = default)",
            "Fireball spacing (0 = default)",
            "Arm count (0 = 2)",
            "Collisions (0 = arm tips only, nonzero = all)",
            P6,
            P7,
            P8,
        ],
        0x106b => [
            P1,
            "Rotation speed (value x 0.001; 0 = default)",
            "Vertical motion speed modifier",
            "Circle radius (0 = 400)",
            P5,
            P6,
            P7,
            P8,
        ],
        0x0d49 => [
            P1,
            P2,
            P3,
            P4,
            P5,
            "Oscillation angle step (0 = 32)",
            "Low water level (0 = 450)",
            "High water level (0 = 800)",
        ],
        _ => UNKNOWN_PARAMETERS,
    }
}

/// Display name for a retail course ID from the course-object header.
/// Debug, battle-slot, reverse-slot, and unknown IDs deliberately return None.
pub const fn course_name(course_id: u8) -> Option<&'static str> {
    match course_id {
        0x21 => Some("Baby Park"),
        0x22 => Some("Peach Beach"),
        0x23 => Some("Daisy Cruiser"),
        0x24 => Some("Luigi Circuit"),
        0x25 => Some("Mario Circuit"),
        0x26 => Some("Yoshi Circuit"),
        0x28 => Some("Mushroom Bridge"),
        0x29 => Some("Mushroom City"),
        0x2a => Some("Waluigi Stadium"),
        0x2b => Some("Wario Colosseum"),
        0x2c => Some("Dino Dino Jungle"),
        0x2d => Some("DK Mountain"),
        0x2f => Some("Bowser's Castle"),
        0x31 => Some("Rainbow Road"),
        0x32 => Some("Dry Dry Desert"),
        0x33 => Some("Sherbet Land"),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn every_id_is_unique_and_resolves() {
        for (index, info) in OBJECTS.iter().enumerate() {
            assert!(OBJECTS[..index].iter().all(|other| other.id != info.id));
            assert_eq!(object_info(info.id), Some(info));
            assert_eq!(object_name(info.id), Some(info.name));
        }
        assert!(object_info(0xffff).is_none());
    }

    #[test]
    fn helpers_cover_common_types_and_courses() {
        assert_eq!(parameter_labels(0x0fa9)[2], "Body segment count");
        assert_eq!(parameter_labels(0x0d49)[7], "High water level (0 = 800)");
        assert_eq!(parameter_labels(0x1198), UNKNOWN_PARAMETERS);
        assert!(!object_info(0x0da4).unwrap().placeable);
        assert_eq!(course_name(0x2d), Some("DK Mountain"));
        assert_eq!(course_name(0), None);
    }

    #[test]
    fn object_models_follow_decomp_file_names() {
        assert_eq!(model_names(0x0001)[0], "itembox_normal");
        assert_eq!(model_names(0x0e75), ["mariotree1"]);
        assert_eq!(model_names(0x1070), ["wl_dokan1"]);
        assert!(model_names(0x0009).contains(&"startline"));
        for (class_name, models) in CLASS_MODELS.iter().chain(INFERRED_CLASS_MODELS) {
            assert!(!models.is_empty(), "{class_name} lists no models");
            assert!(models.iter().all(|name| *name == name.to_ascii_lowercase()));
        }
    }
}
