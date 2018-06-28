#OBJS specifies which files to compile as part of the project
OBJS = caret.cpp console.cpp debug.cpp floattext.cpp game.cpp input.cpp inventory.cpp main.cpp map.cpp map_system.cpp NetPlayer.cpp niku.cpp nx_math.cpp object.cpp ObjManager.cpp p_arms.cpp player.cpp playerstats.cpp profile.cpp ResourceManager.cpp screeneffect.cpp settings.cpp slope.cpp stageboss.cpp stagedata.cpp statusbar.cpp trig.cpp tsc.cpp ai\ai.cpp ai\balrog_common.cpp ai\IrregularBBox.cpp ai\weed\balrog_boss_flying.cpp ai\weed\frenzied_mimiga.cpp ai\weed\weed.cpp ai\weapons\blade.cpp ai\weapons\bubbler.cpp ai\weapons\fireball.cpp ai\weapons\missile.cpp ai\weapons\nemesis.cpp ai\weapons\polar_mgun.cpp ai\weapons\snake.cpp ai\weapons\spur.cpp ai\weapons\weapons.cpp ai\weapons\whimstar.cpp ai\village\balrog_boss_running.cpp ai\village\ma_pignon.cpp ai\village\village.cpp ai\sym\smoke.cpp ai\sym\sym.cpp ai\sand\curly_boss.cpp ai\sand\puppy.cpp ai\sand\sand.cpp ai\sand\toroko_frenzied.cpp ai\plantation\plantation.cpp ai\oside\oside.cpp ai\npc\balrog.cpp ai\npc\curly.cpp ai\npc\curly_ai.cpp ai\npc\misery.cpp ai\npc\npcguest.cpp ai\npc\npcplayer.cpp ai\npc\npcregu.cpp ai\maze\balrog_boss_missiles.cpp ai\maze\critter_purple.cpp ai\maze\gaudi.cpp ai\maze\labyrinth_m.cpp ai\maze\maze.cpp ai\maze\pooh_black.cpp ai\last_cave\last_cave.cpp ai\hell\ballos_misc.cpp ai\hell\ballos_priest.cpp ai\hell\hell.cpp ai\first_cave\first_cave.cpp ai\final_battle\balcony.cpp ai\final_battle\doctor.cpp ai\final_battle\doctor.cpp ai\final_battle\doctor_common.cpp ai\final_battle\doctor_frenzied.cpp ai\final_battle\final_misc.cpp ai\final_battle\misery.cpp ai\final_battle\sidekicks.cpp ai\egg\egg.cpp ai\egg\egg2.cpp ai\egg\igor.cpp ai\boss\balfrog.cpp ai\boss\ballos.cpp ai\boss\core.cpp ai\boss\heavypress.cpp ai\boss\ironhead.cpp ai\boss\omega.cpp ai\boss\sisters.cpp ai\boss\undead_core.cpp ai\boss\x.cpp ai\almond\almond.cpp autogen\AssignSprites.cpp common\glob.cpp common\InitList.cpp common\misc.cpp common\stat.cpp endgame\credits.cpp endgame\CredReader.cpp endgame\island.cpp endgame\misc.cpp graphics\bmfont.cpp graphics\font.cpp graphics\graphics.cpp graphics\nxsurface.cpp graphics\pngfuncs.cpp graphics\screenshot.cpp graphics\sprites.cpp graphics\tileset.cpp intro\intro.cpp intro\title.cpp pause\dialog.cpp pause\message.cpp pause\objects.cpp pause\options.cpp pause\pause.cpp siflib\sectSprites.cpp siflib\sectStringArray.cpp siflib\sif.cpp siflib\sifloader.cpp sound\ogg11.cpp sound\org.cpp sound\pxt.cpp sound\sound.cpp TextBox\ItemImage.cpp TextBox\SaveSelect.cpp TextBox\StageSelect.cpp TextBox\TextBox.cpp TextBox\YesNoPrompt.cpp

#CC specifies which compiler we're using
CC = g++

#INCLUDE_PATHS specifies the additional include paths we'll need
INCLUDE_PATHS = -IC:\Users\Yack\Desktop\sdl2mingw\include\SDL2 -IC:\Users\Yack\Desktop\files2\zlib\libpng-1.6.19

#LIBRARY_PATHS specifies the additional library paths we'll need
LIBRARY_PATHS = -LC:\Users\Yack\Desktop\sdl2mingw\lib -LC:\Users\Yack\Desktop\files2\zlib\libpng-1.6.19\.libs

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
# -Wl,-subsystem,windows gets rid of the console window
# -g, compile for gdb
COMPILER_FLAGS = -std=gnu++11
#-Wl,-subsystem,windows

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = engine

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
