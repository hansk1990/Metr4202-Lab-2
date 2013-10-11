
if ~exist('n_ima'),
   fprintf(1,'No data to process.\n');
   return;
end;

if n_ima == 0,
    fprintf(1,'No image data available\n');
    return;
end;

if ~exist('active_images'),
	active_images = ones(1,n_ima);
end;
n_act = length(active_images);
if n_act < n_ima,
   active_images = [active_images ones(1,n_ima-n_act)];
else
   if n_act > n_ima,
      active_images = active_images(1:n_ima);
   end;
end;

ind_active = find(active_images);

   if length(ind_active)==0,
      %fprintf(1,'\nYou probably want to add images\n');
      choice = 1;
   else
      if length(ind_active)==n_ima,
         %fprintf(1,'\nYou probably want to suppress images\n');
         choice = 0;
      else
         choice = 2;
      end;
   end;
   
while (choice~=0)&(choice~=1),
   choice = input('For suppressing images enter 0, for adding images enter 1 ([]=no change): ');
   if isempty(choice),
      fprintf(1,'No change applied to the list of active images.\n');
      return;
   end;
   if (choice~=0)&(choice~=1),
      disp('Bad entry. Try again.');
   end;
end;


if choice,
   
	ima_numbers = length(ind_active)+1;

if isempty(ima_numbers),
   	ima_proc = 1:n_ima;
	else
   	ima_proc = ima_numbers;
	end;
   
else
   
   
	ima_numbers = length(ind_active)+1;

	if isempty(ima_numbers),
      fprintf(1,'No image has been suppressed. No modication of the list of active images.\n',n_ima);
   	ima_proc = [];
	else
   	ima_proc = ima_numbers;
	end;
   
end;

if ~isempty(ima_proc),
   
   active_images(ima_proc) = choice * ones(1,length(ima_proc));
   
end;


   check_active_images;
   

   fprintf(1,'\nThere is now a total of %d active images for calibration:\n',length(ind_active));
   
   
   