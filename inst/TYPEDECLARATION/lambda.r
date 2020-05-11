type `%:=%` <any, any> => null;
type `%::%` <any, any> => null;
type `add_type` <character, list<class<`data.frame`> | character | null | class<`data.frame`> | character>> => null;
type `add_variant` <character, list<any>, environment> => null;
type `%as%` <any, any> => null;
type `body_fn` <class<`data.frame`> | null, class<`data.frame`>, environment> => (character[] | class<`function`>);
type `check_types` <null | list<class<`data.frame`> | character | integer[]>, any> => logical;
type `clean_defaults` <list<any>> => (^character[] | ^logical[] | null);
type `clean_tokens` <list<any>> => (character[] | null);
type `dereference_type` <character[], list<character[]>> => class<`function`>;
type `fast_forward` <class<`function`>, character[]> => class<`data.frame`>;
type `fill_args` <list<any>, null | character[], any, integer[]> => (null | list<any>);
type `from_root_env` <pairlist> => logical;
type `get_function_env` < > => environment;
type `get_lr` <character> => (class<`function`, `lambdar.fun`> | class<`function`, `lambdar.type`>);
type `get_name` <class<`function`>> => character;
type `get_type` <any, null | double> => (null | list<class<`data.frame`> | character | integer[]>);
type `get_type_index` <class<`function`, `lambdar.fun`> | class<`function`, `lambdar.type`>, any, any> => (double | null);
type `get_variant` <class<`function`, `lambdar.fun`> | class<`function`, `lambdar.type`>, integer> => list<list<any>>;
type `guard_fn` <class<`data.frame`>, class<`data.frame`> | null, any> => (class<`function`> | null);
type `has_ellipsis` <character[]> => integer[];
type `has_variant` <list<any>, any, any, any> => (integer | logical[] | list<any>);
type `idx_ellipsis` <list<any>> => integer[];
type `init_function` <character, environment> => (class<`function`, `lambdar.fun`> | class<`function`, `lambdar.type`>);
type `%isa%` <double, any> => logical;
type `is.bound` <character> => logical;
type `is.infix` <character> => logical;
type `.is.simple` <character | class<`matrix`> | class<`Prices`, `numeric`> | double> => logical;
type `is.type` <character> => logical;
type `iterator` <class<`data.frame`>> => class<`function`>;
type `NewObject` <any, character, ...> => any;
type `parse_body` <class<`function`>> => class<`data.frame`>;
type `parse_eval` <class<`function`>, null> => class<`data.frame`>;
type `parse_fun` <class<`function`>, null> => class<`data.frame`>;
type `parse_guard` <class<`function`>> => (class<`data.frame`> | null);
type `parse_infix` <class<`data.frame`>> => (class<`data.frame`> | null);
type `parse_types` <class<`function`>, class<`data.frame`> | null, any> => class<`data.frame`>;
type `print.lambdar.fun` <class<`function`, `lambdar.fun`>, ...> => null;
type `return_type` <list<class<`data.frame`> | character | integer[]>, list<any>, any> => character;
type `seal` <class<`function`, `lambdar.fun`> | class<`function`, `lambdar.type`>> => null;
type `setup_parent` <character, environment> => (class<`function`, `lambdar.fun`> | class<`function`, `lambdar.type`> | null);
type `strip_ellipsis` <character> => character;
type `.sync_debug` <any> => null;
type `target_env` <class<`call`>, integer> => environment;
type `transform_attrs` <class<`data.frame`> | null> => (class<`data.frame`> | null);
type `update_type_map` <list<any>, character, any> => list<character[]>;
type `use_error` <character, character, list<any>> => character;
type `UseFunction` <any, character, ...> => any;
