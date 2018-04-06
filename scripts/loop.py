from map import * ## "Map Algebra Compiler" package
import sys

# TODO: why only the first Head task is forwarded() ?

setupDevices("",DEV_GPU,"")
#setNumRanks(1)

## Arguments

assert len(sys.argv) > 2
in_file_path = sys.argv[1]
out_file_path = sys.argv[2]

## Methods

inf  = float("inf")
nbh0 = [0, 1, 1, 0,-1,-1,-1, 0, 1]
nbh1 = [0, 0, 1, 1, 1, 0,-1,-1,-1]
dire = [0, 1, 2, 4, 8,16,32,64,128]

def border(raster):
	ds = raster.datasize()
	idx1,idx2 = index(raster,D1), index(raster,D2)
	brd0 = bor( (idx1 == 0) , (idx1 == ds[0]-1) )
	brd1 = bor( (idx2 == 0) , (idx2 == ds[1]-1) )
	return bor( brd0 , brd1 )

def outside(raster,ngb):
	ds = raster.datasize()
	idx1,idx2 = index(raster,D1), index(raster,D2)
	out0 = bor( (idx1+ngb[0] < 0) , (idx1+ngb[0] >= ds[0]) )
	out1 = bor( (idx2+ngb[1] < 0) , (idx2+ngb[1] >= ds[1]) )
	return bor( out0 , out1 )

def pitFill(orig, stream):
	acti = bor( stream , border(orig) )
	elev = con(acti, orig, +inf)
	nbh = [[1,1,1],[1,0,1],[1,1,1]]

	orig = stats(orig) # @
	elev = stats(elev) # @

#... continue ... inject the stats automatically

	#for i in range(int(sys.argv[3])):
	#while value(zor(acti)):
	while zor(acti):
		newe = max( fmin( elev , nbh ) , orig )
		acti = elev != newe
		elev = newe
	return elev

def flowDir(dem,stream):
	maxval = zeros()
	maxdir = zeros([],U8)

	for i in range(1,9):
		ngb = [nbh0[i],nbh1[i]]
		# flows to the steepest NeiGhBor
		zdif = dem - dem(ngb)
		dist = con(i%2!=0, 1., 1.414213)
		drop = zdif / dist
		# borders always flow out
		drop[outside(dem,ngb)] = +inf
		# streams also attract the flow
		drop[stream] = +inf
		# max. drop sets the direction
		maxdir[drop > maxval] = dire[i]
		maxval[drop > maxval] = drop
	return maxdir

def flowDirFlat(flow):
	while flow==0:
		for i in range(1,9):
			ngb = [nbh0[i],nbh1[i]]
			cond = (flow == 0) * (flow(ngb) != 0)
			flow[cond] = dire[i]
	return flow

def dir2coord(flow):
	i = con(flow==0, 0, log2(flow)+1)
	return (pick(i,nbh0),pick(i,nbh1))
# dir2coord

def catchAssign(flow,stream):
	catch = stream
	ngb = dir2coord(flow)

	while catch==0:
		cond = (catch == 0) * (catch(ngb) != 0)
		catch[cond] = catch(ngb)
	return catch

def basinBorder(catch):
	border = zeros_like(catch,U8)
	for i in range(1,9):
		ngb = [nbh0[i],nbh1[i]]
		cond = catch != catch(ngb)
		border[cond] = True
	return border

## Computation

dem = read(in_file_path)
stream = dem == 1
pit = pitFill(dem,stream)
flow = flowDir(pit,stream)
#
write( flow, out_file_path)
sys.exit()
#
flow = flowDirFlat(flow)
catch = catchAssign(flow,stream)
basin = basinBorder(catch)
write(basin, out_file_path)