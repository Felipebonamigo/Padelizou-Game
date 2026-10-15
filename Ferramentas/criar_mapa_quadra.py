# Cria /Game/Mapas/QuadraCinza (pedido 0002): luzes, chão externo, o GameMode sem pawn e o APadelPartidaAtor.
# A quadra, os jogadores, a bola, a câmera de TV e o placar são desenhados pelo ator em tempo de jogo.
# Precisa dos materiais de Ferramentas/criar_materiais.py. Se o mapa já existe, apague Content/Mapas/QuadraCinza.umap.
# Rodar: UnrealEditor-Cmd PadelizouGame.uproject -ExecutePythonScript=Ferramentas/criar_mapa_quadra.py -unattended -nullrhi
import unreal

CAMINHO = "/Game/Mapas/QuadraCinza"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
aes = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not les.new_level(CAMINHO):
    raise RuntimeError("new_level falhou: " + CAMINHO)

def spawn(classe, loc, rot=unreal.Rotator(0, 0, 0)):
    ator = aes.spawn_actor_from_class(classe, loc, rot)
    if ator is None:
        raise RuntimeError("spawn falhou: " + str(classe))
    return ator

# Chão externo (a quadra tem 20 x 10 m; o plano básico tem 1 x 1 m): grande o bastante para o horizonte nunca mostrar o vazio.
fora = spawn(unreal.StaticMeshActor, unreal.Vector(0, 0, -2))
fora.set_actor_label("ChaoExterno")
fora.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane.Plane"))
fora.set_actor_scale3d(unreal.Vector(5000, 5000, 1))
fora.static_mesh_component.set_material(0, unreal.load_asset("/Game/Materiais/M_Cor"))

sol = spawn(unreal.DirectionalLight, unreal.Vector(0, 0, 1500), unreal.Rotator(0, -52, 35))
sol.set_actor_label("Sol")
sol.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property("atmosphere_sun_light", True)
spawn(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0)).set_actor_label("Atmosfera")
ceu = spawn(unreal.SkyLight, unreal.Vector(0, 0, 1700))
ceu.set_actor_label("Ceu")
ceu.get_component_by_class(unreal.SkyLightComponent).set_editor_property("real_time_capture", True)

# O ator da partida: tudo o mais nasce dele.
ator = spawn(unreal.load_class(None, "/Script/PadelizouGame.PadelPartidaAtor"), unreal.Vector(0, 0, 0))
ator.set_actor_label("Partida")

# GameMode sem pawn (a câmera e o controle são do ator).
mundo = unreal.EditorLevelLibrary.get_editor_world()
ajustes = mundo.get_world_settings()
ajustes.set_editor_property("default_game_mode", unreal.load_class(None, "/Script/PadelizouGame.PadelGameMode"))

if not les.save_current_level():
    raise RuntimeError("save_current_level falhou")
unreal.log("Mapa " + CAMINHO + " salvo")
