# Cria os materiais da quadra cinza em /Game/Materiais: M_Cor (opaco, parâmetro Cor) e M_Vidro (translúcido, Cor + Opacidade).
# Rodar: UnrealEditor-Cmd PadelizouGame.uproject -ExecutePythonScript=Ferramentas/criar_materiais.py -unattended -nullrhi
import unreal

mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
PASTA = "/Game/Materiais"

def novo(nome):
    caminho = PASTA + "/" + nome
    if unreal.EditorAssetLibrary.does_asset_exist(caminho):
        unreal.EditorAssetLibrary.delete_asset(caminho)
    return tools.create_asset(nome, PASTA, unreal.Material, unreal.MaterialFactoryNew())

def param_vetor(mat, nome, padrao, x, y):
    n = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", nome)
    n.set_editor_property("default_value", padrao)
    return n

# M_Cor: cor base e um pouco de rugosidade.
m = novo("M_Cor")
cor = param_vetor(m, "Cor", unreal.LinearColor(0.35, 0.35, 0.35, 1), -400, 0)
mel.connect_material_property(cor, "", unreal.MaterialProperty.MP_BASE_COLOR)
rug = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 200)
rug.set_editor_property("r", 0.85)
mel.connect_material_property(rug, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)

# M_Vidro: translúcido.
v = novo("M_Vidro")
v.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
cv = param_vetor(v, "Cor", unreal.LinearColor(0.55, 0.65, 0.7, 1), -400, 0)
mel.connect_material_property(cv, "", unreal.MaterialProperty.MP_BASE_COLOR)
op = mel.create_material_expression(v, unreal.MaterialExpressionScalarParameter, -400, 200)
op.set_editor_property("parameter_name", "Opacidade")
op.set_editor_property("default_value", 0.22)
mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
r2 = mel.create_material_expression(v, unreal.MaterialExpressionConstant, -400, 400)
r2.set_editor_property("r", 0.05)
mel.connect_material_property(r2, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(v)
unreal.EditorAssetLibrary.save_loaded_asset(v)
unreal.log("Materiais salvos em " + PASTA)
