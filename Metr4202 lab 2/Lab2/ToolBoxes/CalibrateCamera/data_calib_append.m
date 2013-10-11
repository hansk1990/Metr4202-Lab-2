%%% This script alets the user enter the name of the images (base name, numbering scheme,...

%Show directory contents
%dir;

Nima_valid = 0;

while (Nima_valid==0),

   calib_name = '';
   
   format_image = 'jpg';      
   check_directory;
   
end;



%string_save = 'save calib_data n_ima type_numbering N_slots image_numbers format_image calib_name first_num';

%eval(string_save);



if (Nima_valid~=0),
    % Reading images:
    
    ima_read_calib_append; % may be launched from the toolbox itself
    % Show all the calibration images:
    
    if ~isempty(ind_read),
        
        mosaic;
        
    end;
    
end;

