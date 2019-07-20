%%
% Plot image maps
%%figure(1)
%%imagesc(rightFarPixelMap)
%%title('rightFarPixelMap','FontSize',13)
 
figure(2)
imagesc(intmap)
title('intmap','FontSize',13)
 
%%
% chosen threshold value; arbitrary  
thresholdvalue=1;
file=intmap;
 
 
sizefile=size(file);
[r,c]=size(file);
 
 
% right traverse
copy=file;
for k=1:c
    for l=1:r
        if file(l,k)<thresholdvalue
            copy(l,k)=0;
        end
    end
end
 
% Plotting the portion of the pixel map above the threshold in yellow 
h=figure(3)
imagesc(copy)
title('Copy','FontSize',13)
print(h,'-loose','-depsc','copyleft.eps')
%% 
% Resaving new image of smaller size for easier processing
sizecopy=size(copy);
[cr,cc]=size(copy);
%right traverse
copytwo=[];
store=0;
for l=1:cr
    for k=1:cc
        if copy(l,k)~=0 && store==0
            store=1;
            startrow=l;
        end
    end
end
store=0;
for l=cr:-1:1
    for k=cc:-1:1
        if copy(l,k)~=0 && store==0
            store=1;
            endrow=l;
        end
    end
end
 
copytwo=copy(startrow:endrow,:);
h=figure(4)
imagesc(copytwo)
title('copyrighttwo','FontSize',13)
print(h,'-loose','-depsc','copyrighttwo.eps')
%%
% Reformatting the cropped figure so that the points above the threshold 
% have a value of 0 and those below the threshold have a value of 1 
% to make for easier processing for the largest square algorithm
copyzeros=copytwo;
[a,b]=size(copytwo);
for k=1:a
    for l=1:b
        if copytwo(k,l)==0
            copyzeros(k,l)=1;
        elseif copytwo(k,l)~=0
            copyzeros(k,l)=0;
        end
    end
end
 
% plotting: pixels above threshold are shown in blue (0) and those below are in
% yellow 
h=figure(5)
imagesc(copyzeros)
title('copyrightzeros','FontSize',13)
print(h,'-loose','-depsc','copyrightzeros.eps')
%%
% Forcing data to a square to fit the largest square algorithm:
 
[a,b]=size(copyzeros);
startcol=b;
endcol=1;
startrow=a;
endrow=1;
 
for j=1:a
    for k=1:b
        if copyzeros(j,k)==0 && k<startcol
            startcol=k;
        end
        if copyzeros(j,k)==0 && k>endcol
            endcol=k;
        end
    end
end
for j=1:b
    for k=1:a
        if copyzeros(k,j)==0 && k<startrow
            startrow=k;
        end
        if copyzeros(k,j)==0 && k>endrow
            endrow=k;
        end
    end
end
 
 
if b>a
    copya=ones(b,b);
    copya(1:a,:)=copyzeros;
elseif a>=b
    copya=ones(a,a);
    copya(:,1:b)=copyzeros;
end
 
width=endcol-startcol+1;
height=endrow-startrow+1;
 
 
 
if height>width
    zerossquare=ones(height);
    zerossquare(:,:)=copya(startrow:endrow,startcol:startcol+height-1);
elseif width>height
    zerossquare=ones(width);
    zerossquare(:,:)=copya(startrow:startrow+width-1,startcol:endcol);
end
 
 
figure(6)
imagesc(zerossquare)
title('Zeros Square','FontSize',13)
print(h,'-loose','-depsc','zerossquare.eps')
 
%%
% Other code I wrote that may help future algorithms: stores the
% largest continuous string above the threshold in each row
sizecopytwo=size(copytwo);
[crt,cct]=size(copytwo);
copythree=zeros(size(copytwo));
%right traverse
largestarea=0;
upperleft=1;
for l=1:crt
    for k=1:cct
        nozeroinrow=1;
        count=0;
        while nozeroinrow==1
            if copytwo(l,k+count)>0
                count=count+1;
            else 
                nozeroinrow=0;
                row=count;
            end
        end
        copythree(l,k)=row;
        
    end
end
 
figure(7)
imagesc(copythree)
title('Largest Continuous String in Row','FontSize',13)
print(h,'-loose','-depsc','copythree.eps')
%%
% Code that takes forever to execute on matrices of this size 
% but works for smaller matrices.
%[r1, r2, c1, c2] = biggest_box(zerossquare)
 
[mm,nn]=size(zerossquare);
 
 
figure(8)
hold on
imagesc([0.5 mm-0.5],[mm-0.5 0.5],zerossquare);
%imagesc(A)
 
xa=r2;
xb=c1;
% mapa=mm-xa;
% mapb=xb-1;
mapa=xb-1;
mapb=mm-xa;
width=c2-c1+1;
height=r2-r1+1;
 
rectangle("position",[mapa mapb width height],"linewidth",3)
 
title('A','FontSize',13)
%print(h,'-loose','-depsc','A.eps')
 
colorbar
 
%%
function [r1, r2, c1, c2] = biggest_box(A)
% get the size (s) of the input square
[a,b] = size(A);
 
% loop through all possible square sizes (n) and positions (r1, c1)
if a<b
    s=a;
else
    s=b;
end
 
for n = s - 1 : -1 : 0 
  for r1 = 1 : a - n
    for c1 = 1 : b - n
      % calculate the position of the lower-right corner
      r2 = r1 + n;
      c2 = c1 + n;
      % return if the summary in the current square is zero
      if sum(sum(A(r1 : r2, c1 : c2))) == 0
        return
      end
    end
  end
end
end

