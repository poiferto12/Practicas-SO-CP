-module(ring).

-export([start/1, send/3, stop/1, loop/2]).

start(N) -> case N of
    0 -> none;
    N ->
        First = spawn(?MODULE, loop, [none, 0]),
        Last = init(First, N-1, 1),
        Last ! {set_next, First},
        First
end.


init(Anterior, 0, _) ->
    Anterior;
init(Anterior, N, Acc) ->
    Siguiente = spawn(?MODULE, loop, [none, Acc]),
    Anterior ! {set_next, Siguiente},
    init(Siguiente, N-1, Acc+1).


loop(Siguiente, Pos) ->
    receive
        {set_next, NuevoSiguiente} ->
            loop(NuevoSiguiente, Pos);
        {mensaje, Msg, Count} ->
            io:format("~p receiving message ~p with ~p left~n", [Pos, Msg, Count-1]),
            case Count-1 of
                0 -> ok;
                _ -> Siguiente ! {mensaje, Msg, Count-1}
            end,
            loop(Siguiente, Pos);
        stop ->
            Siguiente ! stop
    end.


send(Pid, N, Msg) ->
    Pid ! {mensaje, Msg, N},
    ok.


stop(Pid) ->
    Pid ! stop,
    ok.