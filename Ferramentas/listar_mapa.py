import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/Mapas/Fumaca")
aes = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in aes.get_all_level_actors():
    unreal.log("ATOR " + a.get_name() + " " + a.get_class().get_name() + " " + str(a.get_actor_location()))
unreal.log("FIM")
