addpath(genpath('../../matlab-include')) % path to functions
[V,F] = read_triangle_mesh('../../data/argonath.obj'); % read input
% Set parameters
V = V-min(min(V));
V = V./(max(max(V)));
h = 0.005;
bd = 1/.1;
dt = 0.1;
% Set parameters
writeOBJ('argonath_input.obj',V,F); % save input
tStart = tic;
[U,G] = closing_flow(V,F,'Bound',bd,'EdgeLength',h,'TimeStep',dt,...
    'MaxIter',20,'RemeshIterations',2,'Debug',false,'Plot',false,...
    'Write',false); % call method
tStop = toc(tStart);

disp(['closing_flow time: ', num2str(tStop), ' seconds']);

writeOBJ('argonath_output.obj',U,G); % save output



% We've already saved input and output. In order to render them with the
% moving part highlighted, we'll do the following to separate the output
% into an "active" part and an "inactive" one. We then render them as in
% ../../render/render-template.blend
%{
clc; clear all; close all;
[Vgt,Fgt] = readOBJ('argonath_input.obj');
[V,F] = read_triangle_mesh('argonath_output.obj');
[sqrD,I,C] = point_mesh_squared_distance(V,Vgt,Fgt);
A = adjacency_matrix(F) + speye(size(V,1));
moving = find(double(sqrD>1e-6));
%active = find(sum(A(moving,:))>0);
active = moving;
f_active = F(sum(ismember(F,active),2)>2,:);
[I,J,f_active,v_active] = output_sensitive_remove_unreferenced(f_active,V);
f_inactive = F(~(sum(ismember(F,active),2)>2),:);
[Ii,Ji,f_inactive,v_inactive] = output_sensitive_remove_unreferenced(f_inactive,V);
hold off
tsurf(f_active,v_active,'FaceColor',[189,235,252]./255,'EdgeAlpha',0)
hold on
tsurf(f_inactive,v_inactive,'FaceAlpha',0.5,'FaceColor',[.8 .8 .8],'EdgeAlpha',0)
axis equal
camlight
drawnow
%pause
writeOBJ('argonath_active.obj',v_active,f_active);
writeOBJ('argonath_inactive.obj',v_inactive,f_inactive);
%}