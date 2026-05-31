# NixCluster

Simple framework to manage, control and orchestrate multiple Linux devices to run parallelized programs.

How it works:
1. Clients connect to the server and wait for a new task handler.
2. User sends an URL to a Nix Flake to the server. This Flake contains two programs:
 - Server-side - prepares tasks for the clients and sends them to the framework
 - Client-side program generator - Nix function that takes the input of the task and generates the program with embedded input. So client only have to `nix run` said program.
3. Server builds server-side part of the Flake and runs it.
4. Server-side part generates input for client-side part of the Flake.
5. Framework prepares tasks using generated inputs and sends them to free clients (taking into consideration their -march and -mtune config).
6. Clients receive the tasks and builds them.
7. Clients run tasks, get the results from them, and send them back to the server.
8. Clients wait for the next task to run.
