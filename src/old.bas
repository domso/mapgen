randomize timer
screenres 800,600,32
d_iM SHARED AS integer bild_size=512
Dim shared as double map(0 to bild_size,0 to bild_size)
Dim shared as double map_tMP(0 to bild_size,0 to bild_size)
Dim shared as double map2(0 to bild_size,0 to bild_size)
DIm shared as any ptr bild
DIm shared as any ptr bild2
bild=imagecreate(bild_size,bild_size)
bild2=imagecreate(bild_size,bild_size)


for i as integer = 0 to bild_size
for j as integer = 0 to bild_size
map(i,j)=100
next
next


Sub hill(x as integer,y as integer,size as integer,min as integer,max as integer)
	Dim as integer set_height=int(rnd*max-min)+min,diff=1
	
	if int(rnd*2)=1 then diff=-1
	
	if sgn(map(x,y))<>sgn(map(x,y)+diff*set_height) and sgn(map(x,y))=sgn(diff*set_height) then
		'map(x,y)
	else
		map(x,y)+=diff*set_height
	end if
	
	
	'if map(x,y)>255 then map(x,y)=255	
	'if map(x,y)<0 then map(x,y)=0	
	if size mod 2 = 1 then size-=1	
	   
	for i as integer = 0 to size		
	   for j as integer = 0 to size		
			if x-size/2+i>=0 and x-size/2+i<=bild_size and y-size/2+j>=0 and y-size/2+j<=bild_size then 	
				if x- /2+i<>x or y-size/2+j<>y then			
					if set_height>(set_height/(size/2))*(abs(x-(x-size/2+i))^2+abs(y-(y-size/2+j))^2)^0.5  then	

			'			map(x-size/2+i,y-size/2+j)+=diff*(set_height-(set_height/(size/2))*(abs(x-(x-size/2+i))^2+abs(y-(y-size/2+j))^2)^0.5)	
						DIm as double tmp = diff*(set_height-(set_height/(size/2))*(abs(x-(x-size/2+i))^2+abs(y-(y-size/2+j))^2)^0.5)
						
						if sgn(map(x-size/2+i,y-size/2+j))<>sgn(map(x-size/2+i,y-size/2+j)+tmp) and sgn(map(x-size/2+i,y-size/2+j))=sgn(tmp) then
							'map(x,y)
							windowtitle str(x)+"/"+str(y)
						else
							map(x-size/2+i,y-size/2+j)+=tmp
						end if
						
							
						'if map(x-size/2+i,y-size/2+j)>255 then map(x-size/2+i,y-size/2+j)=255				
						'if map(x-size/2+i,y-size/2+j)<0 then map(x-size/2+i,y-size/2+j)=0		
					End if	
				end if	
			end if	
		next
	next
end sub
	/'   			
	  		   	for i as integer = 0 to bild_size
	   	for j as integer = 0 to bild_size		
	   		map(i,j)=20+int(rnd*5)	
	   		next
	   		next
	   	'/	
	   	for i as integer = 0 to (bild_size/512)-1
			for j as integer = 0 to (bild_size/512)-1
					for k as integer = 1 to 1000
						hill(int(rnd*512)+i*512,int(rnd*512)+j*512,int(rnd*200),2,3)
					next
			next
	   	Next
	 /'  		
	   	for i as integer = 0 to 0
			for j as integer = 0 to 0
				for k as integer = 1 to 1000
					hill(int(rnd*bild_size),int(rnd*bild_size),int(rnd*200),10,20-int(k/200))
					'hill(i*512+int(rnd*512),j*512+int(rnd*512),int(rnd*200),10,20-int(k/500))
					hill(i*512+int(rnd*512),j*512+int(rnd*512),int(rnd*200),2,3)
				next
				windowtitle str(j)
			next
			
		next
		
			   			for k as integer = 1 to 1000
					hill(int(rnd*512),int(rnd*512),int(rnd*200),20,30)
				next
		
		'/
		for i as integer = 0 to bild_size
			for j as integer = 0 to bild_size
			'	map(i,j)=20+int(rnd*5)	
				DIm as integer tmp,tmp2
				for ti as integer = -2 to 2
					for tj as integer = -2 to 2
						If i + ti >= 0 And i + ti < bild_size And j + tj >= 0 And j + tj < bild_size then
							if tmp=0 or tmp>map(i+ti,j+tj) then tmp=map(i+ti,j+tj)
							if tmp2=0 or tmp2<map(i+ti,j+tj) then tmp2=map(i+ti,j+tj)
						End if
					next
				next
				map2(i,j)=tmp2-tmp
	   		next
		Next
		
		
		dim as ubyte set_min,set_max
		Dim as double min
		dim as double max
			for i as integer = 0 to bild_size	
				for j as integer = 0 to bild_size
					if set_min=0 then
						min = map(i,j)
						set_min = 1
					else
						if map(i,j)<min then
							min = map(i,j)
						end if
					end if
					if set_max=0 then
						max = map(i,j)
						set_max = 1
					else
						if map(i,j)>max then
							max = map(i,j)
						end if
					end if
				next
			next	

		for i as integer = 0 to bild_size	
			for j as integer = 0 to bild_size	
				'map(i,j)-=min
					if sgn(map(i,j))<>sgn(map(i,j)-min) and sgn(map(i,j))<>sgn(min) then
						
					else
						map(i,j)-=min
					end if
			next
		next
		dim as double scale = 255/(max-min)
		for i as integer = 0 to bild_size	
			for j as integer = 0 to bild_size	
				map(i,j)*=scale
			next
		next
		
		Dim as double tmp
		for a as integer = 1 to 100
			for i as integer = 1 to 511
				for j as integer = 1 to 511
					'map(i,j) = map(i,j)*map(i,j)*map(i,j)*map(i,j)
					tmp+=map(i-1,j-1)
					tmp+=map(i,j-1)*2
					tmp+=map(i+1,j-1)
					tmp+=map(i-1,j)*2
					tmp+=map(i,j)*4
					tmp+=map(i+1,j)*2
					tmp+=map(i-1,j+1)
					tmp+=map(i,j+1)*2
					tmp+=map(i+1,j+1)
					tmp = tmp / 16
					map_tMP(i,j) = tmp
					tmp = 0
				next
			next
			for i as integer = 1 to 511
				for j as integer = 1 to 511
					map(i,j) = map_tMP(i,j)
				next
			next
			
			for i as integer = 0 to 512
				map(0,i)=map(1,i)
				map(512,i) = map(511,i)
				map(i,0) = map(i,1)
				map(i,512) = map(i,511)
			next
			
		next
		
		


		
dim shared as byte Erosionmap(0 to 512,0 to 512)
dim shared as double Erosionmap2(0 to 512,0 to 512)
dim shared as byte set_erosionmap(0 to 512,0 to 512)

for i as integer = 0 to 512
	for j as integer = 0 to 512
		Erosionmap(i,j)+=1
	next
next

for i as integer = 0 to 512
	for j as integer = 0 to 512
		Erosionmap2(i,j) = map(i,j)
	next
next

		for it as integer = 1 to 2
	for i as integer = 0 to bild_size
		for j as integer = 0 to bild_size
			if Erosionmap(i,j)>0 then
				Dim as double min
				Dim as integer minx,miny
				Dim as byte set
				min = map(i,j)
				minx = i
				miny = j
				for a as integer = -1 to 1
					for b as integer = -1 to 1
						if i+a>=0 and i+a<=512 and j+b>=0 and j+b<=512 then
							if set = 0 and int(rnd*2)=1 then
								min = map(i+a,j+b)
								minx = i+a
								miny = j+b 
								set = 1
							else
								if map(i+a,j+b)<min and int(rnd*2)=1 and (a<>0 or b<>0) then
									min = map(i+a,j+b)
									minx = i+a
									miny = j+b 
								end if
							end if
						end if
						
					next
				next
				if i <> minx or j <> miny then
					Erosionmap(i,j)-=1
					Erosionmap(minx,miny)+=1
					Erosionmap2(minx,miny)*=0.9
					'map(i,j)*=0.90
					Erosionmap2(i,j)*=0.9
					if set_erosionmap(i,j) = 0 then
						'Erosionmap2(i,j) *= 0.7
						set_erosionmap(i,j) = 1
						
					end if
				end if
			end if
		next
	next
next

tmp = 0
		for a as integer = 1 to 10
			for i as integer = 1 to 511
				for j as integer = 1 to 511
					'map(i,j) = map(i,j)*map(i,j)*map(i,j)*map(i,j)
					tmp+=Erosionmap2(i-1,j-1)
					tmp+=Erosionmap2(i,j-1)*2
					tmp+=Erosionmap2(i+1,j-1)
					tmp+=Erosionmap2(i-1,j)*2
					tmp+=Erosionmap2(i,j)*4
					tmp+=Erosionmap2(i+1,j)*2
					tmp+=Erosionmap2(i-1,j+1)
					tmp+=Erosionmap2(i,j+1)*2
					tmp+=Erosionmap2(i+1,j+1)
					tmp = tmp / 16
					map_tMP(i,j) = tmp
					tmp = 0
				next
			next
			for i as integer = 1 to 511
				for j as integer = 1 to 511
					Erosionmap2(i,j) = map_tMP(i,j)
				next
			next
			for i as integer = 0 to 512
				Erosionmap2(0,i)=Erosionmap2(1,i)
				Erosionmap2(512,i) = Erosionmap2(511,i)
				Erosionmap2(i,0) = Erosionmap2(i,1)
				Erosionmap2(i,512) = Erosionmap2(i,511)
			next
			
		next
		
		
		
		set_min = 0
		set_max = 0
		min = 0
		max = 0
			for i as integer = 0 to bild_size	
				for j as integer = 0 to bild_size
					if set_min=0 then
						min = Erosionmap2(i,j)
						set_min = 1
					else
						if Erosionmap2(i,j)<min then
							min = Erosionmap2(i,j)
						end if
					end if
					if set_max=0 then
						max = Erosionmap2(i,j)
						set_max = 1
					else
						if Erosionmap2(i,j)>max then
							max = Erosionmap2(i,j)
						end if
					end if
				next
			next	

		for i as integer = 0 to bild_size	
			for j as integer = 0 to bild_size	
				'map(i,j)-=min
					if sgn(Erosionmap2(i,j))<>sgn(Erosionmap2(i,j)-min) and sgn(Erosionmap2(i,j))<>sgn(min) then
						
					else
						Erosionmap2(i,j)-=min
					end if
			next
		next
		scale = 255/(max-min)
		for i as integer = 0 to bild_size	
			for j as integer = 0 to bild_size	
				Erosionmap2(i,j)*=scale
			next
		next
		for i as integer = 0 to bild_size
			for j as integer = 0 to bild_size	
			'	map(i,j)=20+int(rnd*5)	
				DIm as integer tmp,tmp2
				for ti as integer = -2 to 2
					for tj as integer = -2 to 2
						If i + ti >= 0 And i + ti < bild_size And j + tj >= 0 And j + tj < bild_size Then
							if tmp=0 or tmp>Erosionmap2(i+ti,j+tj) then tmp=Erosionmap2(i+ti,j+tj)
							If tmp2=0 or tmp2<Erosionmap2(i+ti,j+tj) then tmp2=Erosionmap2(i+ti,j+tj)
						End If	
					next
				next
				map2(i,j)=tmp2-tmp
	   		next
	   	next
		DIm as double max_diff = sqr(1*bild_size*bild_size) '/2
		
		
		
		for i as integer = 1 to bild_size	
			for j as integer = 1 to bild_size	
				dim as double pdiff = sqr(abs((bild_size/2)-i) * abs((bild_size/2)-i) + abs((bild_size/2)-j) * abs((bild_size/2)-j))
				if ( 1 - ( pdiff / max_diff ) ) > 0 then
					Erosionmap2(i,j)*=( 1 - ( pdiff / max_diff ) )
				else
					Erosionmap2(i,j) = 0
				end if
			next		
		next
		
		windowtitle "finish"
		
		
		for i as integer = 1 to bild_size	
			for j as integer = 1 to bild_size	
				color rgb(Erosionmap2(i,j),Erosionmap2(i,j),Erosionmap2(i,j))
					pset bild,(i-1,j-1)	
			next		
		next
		for i as integer = 1 to bild_size	
			for j as integer = 1 to bild_size	
				color rgba(255,255,255,255-map2(i,j))
					pset bild2,(i-1,j-1)	
			next		
		next
				put (0,0),bild
				bsave "bild.bmp",bild 
				bsave "bild2.bmp",bild2
					sleep

