//==============================================================
//==============================================================
//= terrain.cpp ================================================
//= Original coders: Trent Polack (trent@voxelsoft.com)		   =
//==============================================================
//= This file contains the information for the "parent" terrain=
//= class, which all specific implementations are derived from,=
//= also contains other general data structures and constants. =
//==============================================================
//==============================================================


//--------------------------------------------------------------
//--------------------------------------------------------------
//- HEADERS AND LIBRARIES --------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
#include <stdio.h>
#include <math.h>

#include "../Base Code/gl_app.h"

#include "terrain.h"


//--------------------------------------------------------------
//--------------------------------------------------------------
//- DEFINITIONS ------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------

//--------------------------------------------------------------
// Name:			CTERRAIN::LoadHeightMap - public
// Description:		Load a grayscale RAW height map
// Arguments:		-szFilename: the file name of the height map
//					-im_iSize: the m_iSize (power of 2) of the map
// Return Value:	A boolean value: -true: successful load
//									 -false: unsuccessful load
//--------------------------------------------------------------
bool CTERRAIN::LoadHeightMap( char* szFilename, int iSize )
{
	FILE* pFile;

	//check to see if the data has been set
	if( m_heightData.m_ucpData )
		UnloadHeightMap( );

	//open the RAW height map dataset
	pFile= fopen( szFilename, "rb" );
	if( pFile==NULL )
	{
		//bad filename
		g_log.Write( LOG_FAILURE, "Could not load %s\n", szFilename );
		return false;
	}

	//allocate the memory for our height data
	m_heightData.m_ucpData= new unsigned char [iSize*iSize];

	//check to see if memory was successfully allocated
	if( m_heightData.m_ucpData==NULL )
	{
        //the memory could not be allocated something is seriously wrong here
		g_log.Write( LOG_FAILURE, "Could not allocate memory for%s\n", szFilename );
		return false;
	}

	//read the heightmap into context
	fread( m_heightData.m_ucpData, 1, iSize*iSize, pFile );
	
	//Close the file
	fclose( pFile );

	//set the m_iSize data
	m_iSize= iSize;

	//yahoo! The heightmap has been successfully loaded
	g_log.Write( LOG_SUCCESS, "Loaded %s\n", szFilename );
	return true;
}

//--------------------------------------------------------------
// Name:			CTERRAIN::SaveHeightMap - public
// Description:		Save a grayscale RAW height map
// Arguments:		-szFilename: the file name of the height map
// Return Value:	A boolean value: -true: successful save
//									 -false: unsuccessful save
//--------------------------------------------------------------
bool CTERRAIN::SaveHeightMap( char* szFilename )
{
	FILE* pFile;

	//open a file to write to
	pFile= fopen( szFilename, "wb" );
	if( pFile==NULL )
	{
		//bad filename
		g_log.Write( LOG_FAILURE, "Could not create %s\n", szFilename );
		return false;
	}

	//check to see if our height map actually has data in it
	if( m_heightData.m_ucpData==NULL )
	{
		//something is seriously wrong here
		g_log.Write( LOG_FAILURE, "The height data buffer for %s is empty\n", szFilename );
		return false;
	}

	//write the data to the file
	fwrite( m_heightData.m_ucpData, 1, m_iSize*m_iSize, pFile );
	
	//Close the file
	fclose( pFile );

	//w00t w00t! The heightmap has been successfully saved
	g_log.Write( LOG_SUCCESS, "Saved %s\n", szFilename );
	return true;
}

//--------------------------------------------------------------
// Name:			CTERRAIN::UnloadHeightMap - public
// Description:		Unload the class's height map (if there is one)
// Arguments:		None
// Return Value:	A boolean value: -true: successful load
//									 -false: unsuccessful load
//--------------------------------------------------------------
bool CTERRAIN::UnloadHeightMap( void )
{
	//check to see if the data has been set
	if( m_heightData.m_ucpData )
	{
		//delete the data
		delete[] m_heightData.m_ucpData;

		//reset the map dimensions also
		m_iSize= 0;
	}

	//the height map has been unloaded
	g_log.Write( LOG_SUCCESS, "Successfully unloaded the height map\n" );
	return true;
}

//--------------------------------------------------------------
// Name:			CTERRAIN::NormalizeTerrain - private
// Description:		Scale the terrain height values to a range of
//					0-255
// Arguments:		-fpHeightData: the height data buffer
// Return Value:	None
//--------------------------------------------------------------
void CTERRAIN::NormalizeTerrain( float* fpHeightData )
{
	float fMin, fMax;
	float fHeight;
	int i;

	fMin= fpHeightData[0];
	fMax= fpHeightData[0];

	//find the min/max values of the height fTempBuffer
	for( i=1; i<m_iSize*m_iSize; i++ )
	{
		if( fpHeightData[i]>fMax ) 
			fMax= fpHeightData[i];

		else if( fpHeightData[i]<fMin ) 
			fMin= fpHeightData[i];
	}

	//find the range of the altitude
	if( fMax<=fMin )
		return;

	fHeight= fMax-fMin;

	//scale the values to a range of 0-255 (because I like things that way)
	for( i=0; i<m_iSize*m_iSize; i++ )
		fpHeightData[i]= ( ( fpHeightData[i]-fMin )/fHeight )*255.0f;
}

//--------------------------------------------------------------
// Name:			CTERRAIN::FilterHeightBand - private
// Description:		Apply the erosion filter to an individual 
//					band of height values
// Arguments:		-fpBand: the band to be filtered
//					-iStride: how far to advance per pass
//					-iCount: Number of passes to make
//					-fFilter: the filter strength
// Return Value:	None
//--------------------------------------------------------------
void CTERRAIN::FilterHeightBand( float* fpBand, int iStride, int iCount, float fFilter )
{
	float v= fpBand[0];
	int j  = iStride;
	int i;

	//go through the height band and apply the erosion filter
	for( i=0; i<iCount-1; i++ )
	{
		fpBand[j]= fFilter*v + ( 1-fFilter )*fpBand[j];
		
		v = fpBand[j];
		j+= iStride;
	}
}

//--------------------------------------------------------------
// Name:			CTERRAIN::FilterHeightfTempBuffer - private
// Description:		Apply the erosion filter to an entire buffer
//					of height values
// Arguments:		-fpHeightData: the height values to be filtered
//					-fFilter: the filter strength
// Return Value:	None
//--------------------------------------------------------------
void CTERRAIN::FilterHeightField( float* fpHeightData, float fFilter )
{
	int i;

	//erode left to right
	for( i=0; i<m_iSize; i++ )
		FilterHeightBand( &fpHeightData[m_iSize*i], 1, m_iSize, fFilter );
	
	//erode right to left
	for( i=0; i<m_iSize; i++ )
		FilterHeightBand( &fpHeightData[m_iSize*i+m_iSize-1], -1, m_iSize, fFilter );

	//erode top to bottom
	for( i=0; i<m_iSize; i++ )
		FilterHeightBand( &fpHeightData[i], m_iSize, m_iSize, fFilter);

	//erode from bottom to top
	for( i=0; i<m_iSize; i++ )
		FilterHeightBand( &fpHeightData[m_iSize*(m_iSize-1)+i], -m_iSize, m_iSize, fFilter );
}

//--------------------------------------------------------------
// Name:			CTERRAIN::MakeTerrainFault - public
// Description:		Create a height data set using the "Fault Formation"
//					algorithm.  Thanks a lot to Jason Shankel for this code!
// Arguments:		-iSize: Desired size of the height map
//					-iIterations: Number of detail passes to make
//					-iMinDelta, iMaxDelta: the desired min/max heights
//					-iIterationsPerFilter: Number of passes per filter
//					-fFilter: Strength of the filter
// Return Value:	A boolean value: -true: successful creation
//									 -false: unsuccessful creation
//--------------------------------------------------------------
bool CTERRAIN::MakeTerrainFault( int iSize, int iIterations, int iMinDelta, int iMaxDelta, float fFilter )
{
	float* fTempBuffer;
	int iCurrentIteration;
	int iHeight;
	int iRandX1, iRandZ1;
	int iRandX2, iRandZ2;
	int iDirX1, iDirZ1;
	int iDirX2, iDirZ2;
	int x, z;
	int i;

	if( m_heightData.m_ucpData )
		UnloadHeightMap( );

	m_iSize= iSize;

	//allocate the memory for our height data
	m_heightData.m_ucpData= new unsigned char [m_iSize*m_iSize];
	fTempBuffer= new float [m_iSize*m_iSize];

	//check to see if memory was successfully allocated
	if( m_heightData.m_ucpData==NULL )
	{
		//something is seriously wrong here
		g_log.Write( LOG_FAILURE, "Could not allocate memory for height map" );
		return false;
	}

	//check to see if memory was successfully allocated
	if( fTempBuffer==NULL )
	{
		//something is seriously wrong here
		g_log.Write( LOG_FAILURE, "Could not allocate memory for height map" );
		return false;
	}

	//clear the height fTempBuffer
	for( i=0; i<m_iSize*m_iSize; i++ )
		fTempBuffer[i]= 0;

	for( iCurrentIteration=0; iCurrentIteration<iIterations; iCurrentIteration++ )
	{
		//calculate the height range (linear interpolation from iMaxDelta to
		//iMinDelta) for this fault-pass
		iHeight= iMaxDelta - ( ( iMaxDelta-iMinDelta )*iCurrentIteration )/iIterations;
		
		//pick two points at random from the entire height map
		iRandX1= rand( )%m_iSize;
		iRandZ1= rand( )%m_iSize;
		
		//check to make sure that the points are not the same
		do
		{
			iRandX2= rand( )%m_iSize;
			iRandZ2= rand( )%m_iSize;
		} while ( iRandX2==iRandX1 && iRandZ2==iRandZ1 );

		
		//iDirX1, iDirZ1 is a vector going the same direction as the line
		iDirX1= iRandX2-iRandX1;
		iDirZ1= iRandZ2-iRandZ1;
		
		for( z=0; z<m_iSize; z++ )
		{
			for( x=0; x<m_iSize; x++ )
			{
				//iDirX2, iDirZ2 is a vector from iRandX1, iRandZ1 to the current point (in the loop)
				iDirX2= x-iRandX1;
				iDirZ2= z-iRandZ1;
				
				//if the result of ( iDirX2*iDirZ1 - iDirX1*iDirZ2 ) is "up" (above 0), 
				//then raise this point by iHeight
				if( ( iDirX2*iDirZ1 - iDirX1*iDirZ2 )>0 )
					fTempBuffer[( z*m_iSize )+x]+= ( float )iHeight;
			}
		}

		//erode terrain
		FilterHeightField( fTempBuffer, fFilter );
	}

	//normalize the terrain for our purposes
	NormalizeTerrain( fTempBuffer );

	//transfer the terrain into our class's unsigned char height buffer
	for( z=0; z<m_iSize; z++ )
	{
		for( x=0; x<m_iSize; x++ )
			SetHeightAtPoint( ( unsigned char )fTempBuffer[( z*m_iSize )+x], x, z );
	}

	//delete temporary buffer
	if( fTempBuffer )
	{
		//delete the data
		delete[] fTempBuffer;
	}

	return true;
}

//--------------------------------------------------------------
// Name:			CTERRAIN::MakeTerrainPlasma - public
// Description:		Create a height data set using the "Midpoint
//					Displacement" algorithm.  Thanks a lot to 
//					Jason Shankel for this code!
//					Note: this algorithm has limited use, since
//					CLOD algorithms usually require a height map 
//					size of (n^2)+1 x (n^2)+1, and this algorithm
//					can only generate (n^2) x (n^2) maps
// Arguments:		-iSize: Desired size of the height map
//					-fRoughness: Desired roughness of the created map
// Return Value:	A boolean value: -true: successful creation
//									 -false: unsuccessful creation
//--------------------------------------------------------------
bool CTERRAIN::MakeTerrainPlasma( int iSize, float fRoughness )
{
	float* fTempBuffer;
	float fHeight, fHeightReducer;
	int iRectSize= iSize;
	int ni, nj;
	int mi, mj;
	int pmi, pmj;
	int i, j;
	int x, z;

	if( m_heightData.m_ucpData )
		UnloadHeightMap( );

	if( fRoughness<0 )
		fRoughness*= -1;

	fHeight		  = ( float )iRectSize/2;
	fHeightReducer= ( float )pow(2, -1*fRoughness);

	m_iSize= iSize;

	//allocate the memory for our height data
	m_heightData.m_ucpData= new unsigned char [m_iSize*m_iSize];
	fTempBuffer= new float [m_iSize*m_iSize];

	//check to see if memory was successfully allocated
	if( m_heightData.m_ucpData==NULL )
	{
		//something is seriously wrong here
		g_log.Write( LOG_FAILURE, "Could not allocate memory for height map" );
		return false;
	}

	//check to see if memory was successfully allocated
	if( fTempBuffer==NULL )
	{
		//something is seriously wrong here
		g_log.Write( LOG_FAILURE, "Could not allocate memory for height map" );
		return false;
	}

	//set the first value in the height field
	fTempBuffer[0]= 0.0f;

	//being the displacement process
	while( iRectSize>0 )
	{
		/*Diamond step -

		Find the values at the center of the retangles by averaging the values at 
		the corners and adding a random offset:


		a.....b
		.     .  
		.  e  .
		.     .
		c.....d   

		e  = (a+b+c+d)/4 + random

		In the code below:
		a = (i,j)
		b = (ni,j)
		c = (i,nj)
		d = (ni,nj)
		e = (mi,mj)   */
		for( i=0; i<m_iSize; i+=iRectSize )
		{
			for( j=0; j<m_iSize; j+=iRectSize )
			{
				ni= ( i+iRectSize )%m_iSize;
				nj= ( j+iRectSize )%m_iSize;

				mi= ( i+iRectSize/2 );
				mj= ( j+iRectSize/2 );

				fTempBuffer[mi+mj*m_iSize]= ( float )( ( fTempBuffer[i+j*m_iSize] + fTempBuffer[ni+j*m_iSize] + fTempBuffer[i+nj*m_iSize] + fTempBuffer[ni+nj*m_iSize] )/4 + RangedRandom( -fHeight/2, fHeight/2 ) );
			}
		}

		/*Square step -

		Find the values on the left and top sides of each rectangle
		The right and bottom sides are the left and top sides of the neighboring rectangles,
		  so we don't need to calculate them

		The height m_heightData.m_ucpData wraps, so we're never left hanging.  The right side of the last
			rectangle in a row is the left side of the first rectangle in the row.  The bottom
			side of the last rectangle in a column is the top side of the first rectangle in
			the column

              .......
		      .     .
		      .     .
		      .  d  .
		      .     .
		      .     .
		......a..g..b
		.     .     .
		.     .     .
		.  e  h  f  .
		.     .     .
		.     .     .
		......c......

		g = (d+f+a+b)/4 + random
		h = (a+c+e+f)/4 + random
		
		In the code below:
			a = (i,j) 
			b = (ni,j) 
			c = (i,nj) 
			d = (mi,pmj) 
			e = (pmi,mj) 
			f = (mi,mj) 
			g = (mi,j)
			h = (i,mj)*/
		for( i=0; i<m_iSize; i+=iRectSize )
		{
			for( j=0; j<m_iSize; j+=iRectSize )
			{

				ni= (i+iRectSize)%m_iSize;
				nj= (j+iRectSize)%m_iSize;

				mi= (i+iRectSize/2);
				mj= (j+iRectSize/2);

				pmi= (i-iRectSize/2+m_iSize)%m_iSize;
				pmj= (j-iRectSize/2+m_iSize)%m_iSize;
				
				//Calculate the square value for the top side of the rectangle
				fTempBuffer[mi+j*m_iSize]= ( float )( ( fTempBuffer[i+j*m_iSize]	  +
														fTempBuffer[ni+j*m_iSize]	  +
														fTempBuffer[mi+pmj*m_iSize]	  +
														fTempBuffer[mi+mj*m_iSize] )/4+
														RangedRandom( -fHeight/2, fHeight/2 ) );

				//Calculate the square value for the left side of the rectangle
				fTempBuffer[i+mj*m_iSize]= ( float )( ( fTempBuffer[i+j*m_iSize]	  +
														fTempBuffer[i+nj*m_iSize]	  +
														fTempBuffer[pmi+mj*m_iSize]	  +
														fTempBuffer[mi+mj*m_iSize] )/4+ 
														RangedRandom( -fHeight/2, fHeight/2 ) );
			}
		}

		//reduce the rectangle size by two to prepare for the next
		//displacement stage
		iRectSize/= 2;

		//reduce the height by the height reducer
		fHeight*= fHeightReducer;
	}

	//normalize the terrain for our purposes
	NormalizeTerrain( fTempBuffer );

	//transfer the terrain into our class's unsigned char height buffer
	for( z=0; z<m_iSize; z++ )
	{
		for( x=0; x<m_iSize; x++ )
			SetHeightAtPoint( ( unsigned char )fTempBuffer[( z*m_iSize )+x], x, z );
	}

	//delete temporary buffer
	if( fTempBuffer )
	{
		//delete the data
		delete[] fTempBuffer;
	}

	return true;
}

///获取纹理平铺在给定高度上应可见的百分比
//--------------------------------------------------------------
// Name:			CTERRAIN::RegionPercent - public
// Description:		Get the percentage of which a texture tile should be
//					visible at a given height
// Arguments:		-tileType: type of tile to check
//					-ucHeight: the current height to test for
// Return Value:	A floating point value: the percentage of which the
//					current texture occupies at the given height
//--------------------------------------------------------------
float CTERRAIN::RegionPercent( int tileType, unsigned char ucHeight )
{
	float fTemp1, fTemp2;

	//if the height is lower than the lowest tile's height, then we want full brightness,
	//if we don't do this, the area will get darkened, and no texture will get shown
	if( m_tiles.textureTiles[LOWEST_TILE].IsLoaded( ) )
	{
		if( tileType==LOWEST_TILE && ucHeight<m_tiles.m_regions[LOWEST_TILE].m_iOptimalHeight )
			return 1.0f;//如果当前顶点高度值，低于最低级别的分格图的中位数， 全部用这个最低级别的 分格图
	}
	else if( m_tiles.textureTiles[LOW_TILE].IsLoaded( ) )
	{
		if( tileType==LOW_TILE && ucHeight<m_tiles.m_regions[LOW_TILE].m_iOptimalHeight )
			return 1.0f;
	}
	else if( m_tiles.textureTiles[HIGH_TILE].IsLoaded( ) )
	{
		if( tileType==HIGH_TILE && ucHeight<m_tiles.m_regions[HIGH_TILE].m_iOptimalHeight )
			return 1.0f;
	}
	else if( m_tiles.textureTiles[HIGHEST_TILE].IsLoaded( ) )
	{
		if( tileType==HIGHEST_TILE && ucHeight<m_tiles.m_regions[HIGHEST_TILE].m_iOptimalHeight )
			return 1.0f;
	}

	//height is lower than the region's boundary
	//第二张图进来这里判定，低于第二张图的最低，就是这个第二张图不用显示，就是上面第一张图进来时候，判定走上面，得到1.0，整张图颜色值都要
	if( ucHeight<m_tiles.m_regions[tileType].m_iLowHeight )  
		return 0.0f;

	//height is higher than the region's boundary
	else if( ucHeight>m_tiles.m_regions[tileType].m_iHighHeight )//高度超第二格最大范围 返回0，不用这一格的贴图颜色值，等待第三张图判定
		return 0.0f;

	//height is below the optimum height
	//下面判定是在这一格图范围内，中位数前和后的情况
	if( ucHeight<m_tiles.m_regions[tileType].m_iOptimalHeight ) // m_iLowHeight < height < m_iOptimalHeight
	{
		//calculate the texture percentage for the given tile's region
		fTemp1= ( float )ucHeight-m_tiles.m_regions[tileType].m_iLowHeight;//与低位的差值
		fTemp2= ( float )m_tiles.m_regions[tileType].m_iOptimalHeight-m_tiles.m_regions[tileType].m_iLowHeight;//低位与中位数的差值

		return ( fTemp1/fTemp2 );//求出height在低位与中位数差的百分比，就是这个高度值在这个分格图上，需要显示的颜色值百分比
	}

	//height is exactly the same as the optimal height
	else if( ucHeight==m_tiles.m_regions[tileType].m_iOptimalHeight )
		return 1.0f;

	//height is above the optimal height
	else if( ucHeight>m_tiles.m_regions[tileType].m_iOptimalHeight )// m_iOptimalHeight < height < m_iHighHeight  同样思路
	{
		//calculate the texture percentage for the given tile's region
		fTemp1= ( float )m_tiles.m_regions[tileType].m_iHighHeight-m_tiles.m_regions[tileType].m_iOptimalHeight;
		//但是这里是要 ucHeight 到 m_iHighHeight 的 后半段，就是剩余的占比， 为了满足第三格的和第四格的重叠情况，第四格是超出255作为最大范围的，第四格要占比高
		return ( ( fTemp1-( ucHeight-m_tiles.m_regions[tileType].m_iOptimalHeight ) )/fTemp1 );
	}

	//something is seriously wrong if the height doesn't fit the previous cases
	return 0.0f;
}

//--------------------------------------------------------------
// Name:			CTERRAIN::GetTexCoords - public
// Description:		Get texture coordinates :)
//					present in the final texture
// Arguments:		-texture: the texture to get coordinates for
//					-*x, *y: the unaltered texture coordinates, and the
//							 storage place for the altered coordinates
// Return Value:	None
//--------------------------------------------------------------
void CTERRAIN::GetTexCoords( CIMAGE texture, unsigned int* x, unsigned int* y )
{
	unsigned int uiWidth = texture.GetWidth( );
	unsigned int uiHeight= texture.GetHeight( );
	int iRepeatX= -1;
	int iRepeatY= -1;
	int i= 0;

	//由于合并的贴图尺寸可能比分格贴图大，分格贴图需要平铺，所以要计算重复位置
	//loop until we figure out how many times the tile has repeated (on the X axis)
	while( iRepeatX==-1 )
	{
		i++;

		//if x is less than the total width, then we found a winner!
		if( *x<( uiWidth*i ) )
			iRepeatX= i-1;
	}

	//prepare to figure out the repetition on the Y axis
	i= 0;

	//loop until we figure out how many times the tile has repeated (on the Y axis)
	while( iRepeatY==-1 )
	{
		i++;

		//if y is less than the total height, then we have a bingo!
		if( *y<( uiHeight*i ) )
			iRepeatY= i-1;
	}

	//*指针 方式赋值，把计算平铺次数后的UI值附上
	//update the given texture coordinates
	*x= *x-( uiWidth*iRepeatX );
	*y= *y-( uiHeight*iRepeatY );
}

//插值高度贴图中的高度，以便生成的纹理贴图看起来不会异常块状
//--------------------------------------------------------------
// Name:			CTERRAIN::InterpolateHeight - public
// Description:		Interpolate the heights in the height map so that
//					the generated texture map does not look incredibly blocky
// Arguments:		-x, z: coordinates to get the height at
//					-fHeightToTexRatio: Height map size to texture 
//										map size ratio
// Return Value:	An unsigned char value: the interpolated height
//--------------------------------------------------------------
unsigned char CTERRAIN::InterpolateHeight( int x, int z, float fHeightToTexRatio )
{
	//x z是传入的正在迭代的纹理贴图的像素坐标
	unsigned char ucLow, ucHighX, ucHighZ;//最低值，最高值定义，用于插值运算
	float ucX, ucZ;
	//128 - 256  --> 0.5
	//128 - 64 --> 2
	//256[1] --> 128[0.5]  纹理贴图比高度图要大
	float fScaledX= x*fHeightToTexRatio;//高度图与纹理贴图的比例  256 -> 256 * 0.5 = 128 
	float fScaledZ= z*fHeightToTexRatio;
	float fInterpolation;

	//set the middle boundary
	ucLow= GetTrueHeightAtPoint( ( int )fScaledX, ( int )fScaledZ );//最低值

	//start off by interpolating along the X axis
	//set the high boundary
	if( ( fScaledX+1 )>m_iSize )//已经到达高度图边界
		return ucLow;
	else
		ucHighX= GetTrueHeightAtPoint( ( int )fScaledX+1, ( int )fScaledZ );//还未到达边界，找下一个x方向的高度值作为最高值的意思

	//calculate the interpolation (for the X axis)
	fInterpolation= ( fScaledX-( int )fScaledX );//(int)向下取整
	ucX= ( ( ucHighX-ucLow )*fInterpolation )+ucLow;  //相当于  ： 最低值 + 差值*百分比  就是增加了百分多少

	//interpolate along the Z axis now
	//set the high boundary
	if( ( fScaledZ+1 )>m_iSize )
		return ucLow;
	else
		ucHighZ= GetTrueHeightAtPoint( ( int )fScaledX, ( int )fScaledZ+1 );

	//calculate the interpolation (for the Z axis)
	fInterpolation= ( fScaledZ-( int )fScaledZ );
	ucZ= ( ( ucHighZ-ucLow )*fInterpolation )+ucLow;

	//calculate the overall interpolation (average of the two values)
	return ( ( unsigned char )( ( ucX+ucZ )/2 ) );//求出平均高度值
}

//此算法跟unity的terrain贴图不一样
//从4个格生成纹理，混合贴图到一张
// 4个纹理，最后的m_iHighHeight累加的结果是，255/4 * 4 - 1 ，最后限制在255内
//--------------------------------------------------------------
// Name:			CTERRAIN::GenerateTextureMap - public
// Description:		Generate a texture map from the four tiles (that must
//					be loaded before this function is called)
// Arguments:		-uiSize: the size of the texture map to be generated
// Return Value:	None
//--------------------------------------------------------------
void CTERRAIN::GenerateTextureMap( unsigned int uiSize )
{
	unsigned char ucRed, ucGreen, ucBlue, tInterpHeight;
	unsigned int iTempID;
	unsigned int x, z;
	unsigned int uiTexX, uiTexZ;//采样分格贴图的UV坐标，不是模型的UV
	//模型UV坐标是以地形尺寸按比例换算到每个顶点上，0-1的值，采样整张贴图
	//通过算法计算每个顶点的高度值下，对应的分格贴图的情况，实现不同高度上的贴图不同
	float fTotalRed, fTotalGreen, fTotalBlue;
	float fBlend[4];
	float fMapRatio;
	int iLastHeight;
	int i;

	//find out the number of tiles that we have
	m_tiles.iNumTiles= 0;
	for( i=0; i<TRN_NUM_TILES; i++ )
	{
		//if the current tile is loaded, then we add one to the total tile count
		if( m_tiles.textureTiles[i].IsLoaded( ) )
			m_tiles.iNumTiles++;
	}

	/*
	第一分格  m_iLowHeight = 0  ---   m_iOptimalHeight = 255 * 1/4 - 1   ---   m_iHighHeight = 累计 255 * 1/4 - m_iLowHeight (差值) +  累计 255 * 1/4  加上值后，到边界值


		0  ---- 255/4 -1 ---- 255/4 -1 ---- 255/4 -1 ---- 255/4 -1
	[0]	L          O            H
	[1]		   	   L            O            H
	[2]					        L            O            H
	[3]							             L            O            H  超出的范围不会被判定到，高度值都在L-O 之间

	*/

	//now, re-loop through, and calculate the texture regions
	iLastHeight= -1;
	for( i=0; i<TRN_NUM_TILES; i++ )
	{
		//we only want to perform these calculations if we actually have a tile loaded
		if( m_tiles.textureTiles[i].IsLoaded( ) )
		{
			//calculate the three height boundaries
			m_tiles.m_regions[i].m_iLowHeight= iLastHeight+1;
			iLastHeight+= 255/m_tiles.iNumTiles;

			m_tiles.m_regions[i].m_iOptimalHeight= iLastHeight;

			m_tiles.m_regions[i].m_iHighHeight= ( iLastHeight-m_tiles.m_regions[i].m_iLowHeight )+iLastHeight;
		}
	}

	//create room for a new texture
	m_texture.Create( uiSize, uiSize, 24 );

	//get the height map to texture map ratio (since, most of the time,
	//the texture map will be a higher resolution than the height map, so
	//we need the ratio of height map pixels to texture map pixels)
	fMapRatio= ( float )m_iSize/uiSize;//  128/256

	//time to create the texture data
	for( z=0; z<uiSize; z++ )
	{
		for( x=0; x<uiSize; x++ )
		{
			//set our total color counters to 0.0f
			fTotalRed  = 0.0f;
			fTotalGreen= 0.0f;
			fTotalBlue = 0.0f;
			tInterpHeight = InterpolateHeight(x, z, fMapRatio);//插值算出当前整合纹理的x,z像素下，高度图的高度值

			//loop through the tiles (for the third time in this function!)
			for( i=0; i<TRN_NUM_TILES; i++ )
			{
				//if the tile is loaded, we can do the calculations
				if( m_tiles.textureTiles[i].IsLoaded( ) )
				{
					uiTexX= x;
					uiTexZ= z;

					//get texture coordinates
					GetTexCoords( m_tiles.textureTiles[i], &uiTexX, &uiTexZ );

					//get the current color in the texture at the coordinates that we got
					//in GetTexCoords
					//用计算得到的UI坐标，从分格贴图中获取对应的rgb值，赋予变量  0-255
					m_tiles.textureTiles[i].GetColor( uiTexX, uiTexZ, &ucRed, &ucGreen, &ucBlue );

					//get the current coordinate's blending percentage for this tile
					//获取此分格贴图的当前坐标混合百分比
					fBlend[i]= RegionPercent( i, tInterpHeight );//每一张图，在同一个贴图x,z像素坐标下，的颜色值百分比情况

					//calculate the RGB values that will be used
					fTotalRed  += ucRed*fBlend[i];//当前分格贴图的颜色值 * 其计算得到的百分比，累加到最终的颜色值，才是整合贴图当前x,z像素的颜色值  控制在0-255
					fTotalGreen+= ucGreen*fBlend[i];
					fTotalBlue += ucBlue*fBlend[i];
				}
			}			

			//set our terrain's texture color to the one that we previously calculated
			m_texture.SetColor( x, z, ( unsigned char )fTotalRed,
									  ( unsigned char )fTotalGreen,
									  ( unsigned char )fTotalBlue );
		}
	}

	//绑定到opengl 提交渲染流水线 GPU

	//build the OpenGL texture
	glGenTextures( 1, &iTempID );
	glBindTexture( GL_TEXTURE_2D, iTempID );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );						

	//make the texture
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, uiSize, uiSize, 0, GL_RGB, GL_UNSIGNED_BYTE, m_texture.GetData( ) );

	//set the texture's ID
	m_texture.SetID( iTempID );
}