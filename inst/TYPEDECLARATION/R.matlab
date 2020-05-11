type `as.character.Matlab` <class<`Matlab`, `Object`>, ...> => character;
type `close.Matlab` <class<`Matlab`, `Object`>, ...> => null;
type `constructor` <character, double, logical> => class<`Matlab`, `Object`>;
type `evaluate` <...> => any;
type `evaluate.Matlab` <class<`Matlab`, `Object`>, ..., character, logical> => any;
type `finalize.Matlab` <class<`Matlab`, `Object`>, ...> => null;
type `fun` <any, character> => null | <any, character> => any;
type `getOption.Matlab` <class<`Matlab`, `Object`>, ...> => (double | null);
type `getVariable` <...> => any;
type `getVariable.Matlab` <class<`Matlab`, `Object`>, character, any, ...> => any;
type `isOpen` <...> => logical;
type `isOpen.default` <...> => logical;
type `isOpen.Matlab` <class<`Matlab`, `Object`>, ...> => logical;
type `Matlab` <character, double, logical> => class<`Matlab`, `Object`>;
type `.onAttach` <any, character> => null;
type `.onLoad` <any, character> => any;
type `.onUnload` <any> => class<`matrix`>;
type `open.Matlab` <class<`Matlab`, `Object`>, integer, double, double, ...> => list<any>;
type `readMat` <...> => any;
type `readMat.default` <character | class<`connection`, `file`> | raw[], null, logical, character, character[], double, ...> => list<any>;
type `readResult` <...> => any;
type `readResult.Matlab` <class<`Matlab`, `Object`>, ...> => any;
type `setFunction` <...> => any;
type `setFunction.Matlab` <class<`Matlab`, `Object`>, character, null, character, ...> => list<any>;
type `setOption.Matlab` <class<`Matlab`, `Object`>, ...> => null;
type `setVariable` <...> => any;
type `setVariable.Matlab` <class<`Matlab`, `Object`>, ..., logical> => list<any>;
type `setVerbose` <...> => double;
type `setVerbose.Matlab` <class<`Matlab`, `Object`>, double, ...> => double;
type `startServer` <...> => integer;
type `startServer.Matlab` <class<`Matlab`, `Object`>, null, double, logical, character[], character, ...> => integer;
type `stop_if_not` <...> => null;
type `writeCommand` <...> => any;
type `writeCommand.Matlab` <class<`Matlab`, `Object`>, character, ...> => any;
type `writeMat` <...> => any;
type `writeMat.default` <character | class<`connection`, `file`>, ..., logical, character, class<`function`> | null, any> => any;
