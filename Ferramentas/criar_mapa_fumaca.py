# Cria /Game/Mapas/Fumaca: chão, luz, câmera lateral e o APadelFumacaAtor (pedido 0001).
# Rodar: UnrealEditor-Cmd PadelizouGame.uproject -ExecutePythonScript=Ferramentas/criar_mapa_fumaca.py -unattended -nullrhi
import unreal

CAMINHO = "/Game/Mapas/Fumaca"
nivel = unreal.EditorLevelLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
aes = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Se o mapa já existe, apague Content/Mapas/Fumaca.umap antes (o new_level não sobrescreve).
if not les.new_level(CAMINHO):
    raise RuntimeError("new_level falhou: " + CAMINHO)

def spawn(classe, loc, rot=unreal.Rotator(0, 0, 0)):
    return aes.spawn_actor_from_class(classe, loc, rot)

# Chão: 20 x 10 m (o plano básico tem 100 x 100 cm).
chao = spawn(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
chao.set_actor_label("Chao")
chao.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane.Plane"))
chao.set_actor_scale3d(unreal.Vector(20, 10, 1))

sol = spawn(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(0, -50, 30))
sol.set_actor_label("Sol")
ceu = spawn(unreal.SkyLight, unreal.Vector(0, 0, 1200))
ceu.set_actor_label("Ceu")

# Câmera lateral só para ver o arco da bola (a câmera de TV é o pedido 0002).
cam = spawn(unreal.CameraActor, unreal.Vector(0, 2200, 350), unreal.Rotator(0, 0, -90))
cam.set_actor_label("CameraLateral")
cam.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.PLAYER0)

spawn(unreal.load_class(None, "/Script/PadelizouGame.PadelFumacaAtor"), unreal.Vector(900, 0, 100)).set_actor_label("Fumaca")

if not les.save_current_level():
    raise RuntimeError("save_current_level falhou")
unreal.log("Mapa " + CAMINHO + " salvo")
