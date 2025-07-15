-module(line).

-export([start/1, send/2, stop/1, loop/2]).

start(N) -> case N of
    0 -> none;
    N ->
        First = spawn(?MODULE, loop, [none, 0]),
        init(First, N-1, 1),
        First
end.

init(_, 0, _) ->
    ok;
init(Anterior, N, Acc) ->
    Siguiente = spawn(?MODULE, loop, [none, Acc]),
    Anterior ! {set_next, Siguiente},
    init(Siguiente, N-1, Acc+1).


loop(Siguiente, Acc) ->
    receive
        {set_next, NuevoSiguiente} ->
            loop(NuevoSiguiente, Acc);
        {mensaje, Msg} ->
            io:format("~p received message ~p~n", [Acc, Msg]),
            case Siguiente of
                none -> ok;
                Siguiente -> Siguiente ! {mensaje, Msg}
            end,
            loop(Siguiente, Acc);
        stop ->
            case Siguiente of
                none -> ok;
                Siguiente -> Siguiente ! stop
            end
    end.


send(Pid, Msg) ->
    Pid ! {mensaje, Msg},
    ok.


stop(Pid) ->
    Pid ! stop,
    ok.