#pragma once

#include <gtkwave.h>
#include <fstapi.h>

GwTreeKind fst_scope_type_to_gw_tree_kind(enum fstScopeType scope_type);
GwVarType fst_var_type_to_gw_var_type(enum fstVarType var_type);
GwVarDir fst_var_dir_to_gw_var_dir(enum fstVarDir var_dir);
GwVarDataType fst_supplemental_data_type_to_gw_var_data_type(enum fstSupplementalDataType);
