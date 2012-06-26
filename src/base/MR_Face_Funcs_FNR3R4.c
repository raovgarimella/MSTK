#define _H_MRegion_Private

#include "MRegion.h"
#include "MRegion_jmp.h"
#include "MSTK_private.h"
#include "MSTK_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif


  int MR_Set_GInfo_Auto_FNR3R4(MRegion_ptr r) {
    int i, nf, same, rgdim, rgid, fgdim, fgid, fgdim0, fgid0;
    MFace_ptr f;
    MRegion_Adj_R3R4 *adj;

    adj = (MRegion_Adj_R3R4 *) r->adj;
    nf = List_Num_Entries(adj->rfaces);

    same = 1;
    rgdim = -1;
    rgid = -1;
    fgid = -1;

    f = List_Entry(adj->rfaces,0);    
    fgid0 = MF_GEntID(f);
    fgdim0 = MF_GEntDim(f);

    for (i = 1; i < nf; i++) {
      f = List_Entry(adj->rfaces,i);
      fgid = MF_GEntID(f);
      fgdim = MF_GEntDim(f);
      if (fgdim == fgdim0 && fgid == fgid0)
	continue; /* all edges have same classification so far */
      else {
	same = 0;
	if (fgdim > rgdim) {
	  rgdim = fgdim;
	  rgid = fgid;
	}
      }
    }
    if (same) {
      rgdim = fgdim0;
      rgid = fgid;
    }
     
    if (rgdim == -1)
      rgdim = 4;
    MEnt_Set_GEntDim((MEntity_ptr) r,rgdim);
    MEnt_Set_GEntID((MEntity_ptr) r,rgid);

    if (rgdim == 4)
      return 0;
    else
      return 1;
  }

  void MR_Set_Faces_FNR3R4(MRegion_ptr r, int nf, MFace_ptr *rfaces,int *dirs){
    int i, j, k;
    MRegion_Adj_FN *adj;

    if (nf > MAXPF3)
      MSTK_Report("MR_Set_Faces","Region has too many faces",MSTK_ERROR);

    adj = (MRegion_Adj_FN *) r->adj;
    k = 1 + ((int) nf/(8*sizeof(unsigned int)));
    adj->fdirs = (unsigned int *) MSTK_calloc(k,sizeof(unsigned int));
    adj->rfaces = List_New(nf);

    for (i = 0; i < nf; i++) {

#ifdef DEBUG
      if (MR_Mesh(r) != MF_Mesh(rfaces[i]))
	MSTK_Report("MR_Set_Faces_FNR3R4",
		    "Region and face belong to different meshes",MSTK_FATAL);
#endif

      j = (int) i/(8*sizeof(unsigned int));
      k = i%(8*sizeof(unsigned int));
      adj->fdirs[j] = adj->fdirs[j] | (dirs[i] << k);

      List_Add(adj->rfaces,rfaces[i]);

      if (MEnt_RepType((MEntity_ptr) r) != F4) 
	MF_Add_Region(rfaces[i],r,!dirs[i]);
    }


    /* Figure out which type of element it is and set it */

    if (r->mrtype == RUNKNOWN) {
      switch (nf) {
      case 4: {
        int alltri = 1;
        for (i = 0; i < nf; i++) {
          if (MF_Num_Vertices(rfaces[i]) != 3) {
            alltri = 0;
            break;
          }
        }
        
        if (alltri) 
          r->mrtype = TET;
        else
          r->mrtype = POLYHED;        
        break;
      }
      case 5: {
        int nquads = 0; 
        int ntris = 0;
        
        for (i = 0; i < nf; i++) {
          int nv = MF_Num_Vertices(rfaces[i]);
          if (nv == 3)
            ntris++;
          else if (nv == 4)
            nquads++;
        }
        
        if (nquads == 3 && ntris == 2)
          r->mrtype = PRISM;
        else if (nquads == 1 && ntris == 4)
          r->mrtype = PYRAMID;
        else
          r->mrtype = POLYHED;        
        break;
      }
      case 6: {
        int allquad = 1;
        for (i = 0; i < nf; i++) {
          if (MF_Num_Vertices(rfaces[i]) != 4) {
            allquad = 0;
            break;
          }
        }
        
        if (allquad)
          r->mrtype = HEX;
        else
          r->mrtype = POLYHED;        
        break;
      }
      default: 
        r->mrtype = POLYHED;
        break;
      }
    }
  }

  void MR_Set_Vertices_FNR3R4(MRegion_ptr r, int nv, MVertex_ptr *mvertices, 
                              int nf, int **rfvtemplate) {
    MVertex_ptr verts[MAXPV2], everts[2];
    MEdge_ptr redges[12], fedges[4]; /* will be used for std. elements only */
    MFace_ptr rfaces[MAXPF3];
    MRegion_ptr oregion;
    int rfdirs[MAXPF3], odir, i, j, fgdim, fgid, nfv, ind, nre;
    int vgdim[MAXPV3], vgid[MAXPV3], egdim[12], egid[12], sgn;
    int fedirs[4], redirs[12], evgdim[2], evgid[2], nfe;
    List_ptr fverts, fregs;
    Mesh_ptr mesh = MEnt_Mesh((MEntity_ptr) r);
    MRType regtype;


    for (i = 0; i < nv; i++) {
      vgdim[i] = MV_GEntDim(mvertices[i]);
      vgid[i] = MV_GEntID(mvertices[i]);

#ifdef DEBUG
      if (MR_Mesh(r) != MV_Mesh(mvertices[i]))
	MSTK_Report("MR_Set_Vertices_FNR3R4",
		    "Region and Vertex belong to different meshes",MSTK_FATAL);
#endif
    }

    if (rfvtemplate) {
      switch (nf) {
      case 4: {
        int alltri = 1;
        for (i = 0; i < nf; i++) 
          if (rfvtemplate[i][0] != 3) {
            alltri = 0;
            break;
          }
        if (alltri)
          r->mrtype = TET;
        else
          r->mrtype = POLYHED;
        break;
      }
      case 5: {
        int nquads = 0; 
        int ntris = 0;
        for (i = 0; i < nf; i++)
          if (rfvtemplate[i][0] == 3)
            ntris++;
          else if (rfvtemplate[i][1] == 4)
            nquads++;

        if (nquads == 3 && ntris == 2)
          r->mrtype = PRISM;
        else if (nquads == 1 && ntris == 4)
          r->mrtype = PYRAMID;
        else
          r->mrtype = POLYHED;
        break;
      }
      case 6: {
        int allquad = 1;
        for (i = 0; i < nf; i++)
          if (rfvtemplate[i][0] != 4) {
            allquad = 0;
            break;
          }
        if (allquad)
          r->mrtype = HEX;
        else
          r->mrtype = POLYHED;
        break;
      }
      default:
        r->mrtype = POLYHED;
      }
        
      for (i = 0; i < nf; i++) {
	fgdim = 4;
	fgid = 0;

	nfv = rfvtemplate[i][0];
	for (j = 0; j < nfv; j++) {
	  ind = rfvtemplate[i][j+1];
	  verts[j] = mvertices[ind];
	  if (fgdim == 4 || vgdim[ind] > fgdim) {
	    fgdim = vgdim[ind];
	    fgid = vgid[ind];
	  }
	}

       
	rfaces[i] = MVs_CommonFace(nfv, verts);

	if (!rfaces[i]) {
	  rfaces[i] = MF_New(mesh);

	  if (fgdim >= 2) {
	    MF_Set_GEntDim(rfaces[i],fgdim);
	    MF_Set_GEntID(rfaces[i],fgid);
	  }
	  else {
	    /* Cannot determine unique classification for face */
	    MSTK_Report("MR_Set_Vertices",
			"Cannot determine unique classification for face",
			MSTK_WARN);
	    MF_Set_GEntDim(rfaces[i],4);
	    MF_Set_GEntID(rfaces[i],0);
	  }


	  MF_Set_Vertices(rfaces[i], nfv, verts);
	  rfdirs[i] = 1;
	}
	else {
	  fregs = MF_Regions(rfaces[i]);
	  if (!fregs) {
	    fverts = MF_Vertices(rfaces[i],1,verts[0]);
	    rfdirs[i] = (verts[1] == List_Entry(fverts,1)) ? 1 : 0;
	    List_Delete(fverts);
	  }
	  else {
#ifdef DEBUG
	    if (List_Num_Entries(fregs) > 1)
	      MSTK_Report("MR_Set_Vertices",
			  "Face already connected on both sides",MSTK_FATAL);
#endif
	    oregion = List_Entry(fregs,0);
	    odir = MR_FaceDir(oregion,rfaces[i]);
	    rfdirs[i] = !odir;

	    List_Delete(fregs);
	  }
	}

      }

      MR_Set_Faces(r, nf, rfaces, rfdirs);
    }
    else {
      switch (nv) {
      case 4:
	r->mrtype = TET;
	nf = 4;
	break;
      case 5:
	r->mrtype = PYRAMID;
	nf = 5;
	break;
      case 6:
	r->mrtype = PRISM;
	nf = 5;
	break;
      case 8:
	r->mrtype = HEX;
	nf = 6;
	break;
      default:
	MSTK_Report("MR_Set_Vertices",
		    "Polyhedron: Need number of faces and vertex template",
		    MSTK_FATAL);
	r->mrtype = RUNKNOWN;
	break;
      }

      /* First collect the edges */
      nre = MSTK_nre_template[r->mrtype];
      for (i = 0; i < nre; i++) {

	/* Check if edge exists */

	ind = MSTK_rev_template[r->mrtype][i][0];
	everts[0] = mvertices[ind];
	evgdim[0] = MV_GEntDim(everts[0]);
	evgid[0]  = MV_GEntID(everts[0]);
	
	ind = MSTK_rev_template[r->mrtype][i][1];
	everts[1] = mvertices[ind];
	evgdim[1] = MV_GEntDim(everts[1]);
	evgid[1]  = MV_GEntID(everts[1]);

	redges[i] = MVs_CommonEdge(everts[0],everts[1]);

	/* Create the edge if it does not exist */

	if (!redges[i]) {
	  redges[i] = ME_New(mesh);

	  ME_Set_Vertex(redges[i],0,everts[0]);
	  ME_Set_Vertex(redges[i],1,everts[1]);
	  redirs[i] = 1;

	  egdim[i] = 4;
	  egid[i] = 0;

	  if (evgdim[0] > evgdim[1]) {
	    egdim[i] = evgdim[0];
	    egid[i] = evgid[0];
	  }
	  else if (evgdim[1] > evgdim[0]) {
	    egdim[i] = evgdim[1];
	    egid[i] = evgid[1];
	  }
	  else { /* evgdim[0] == evgdim[1] */
	    if (evgdim[0] == 0) {
	      MSTK_Report("MR_Set_Vertices_FNR3R4",
			  "Cannot determine edge classification. Guessing...",
			  MSTK_WARN);
	      egdim[i] = 1;
	      egid[i] = 0;
	    }
	    else {
	      if (evgid[0] == evgid[1]) {
		/* Assume that edge is classified on same entity as
		   its two vertices. Could be wrong but will be right
		   more often than not */
		egdim[i] = evgdim[0];
		egid[i] = evgid[0];
	      }
	      else {
		/* Vertices are classified on two different edges,
		   faces or regions */
		MSTK_Report("MR_Set_Vertices_FNR3R4",
			    "Cannot determine edge classification. Guessing..",
			    MSTK_WARN);
		egdim[i] = evgdim[0]+1;
		egid[i] = 0;
	      }
	    }
	  }

	  ME_Set_GEntDim(redges[i],egdim[i]);
	  ME_Set_GEntID(redges[i],egid[i]);
	}
	else {
	  redirs[i] = (ME_Vertex(redges[i],0) == everts[0]) ? 1 : 0;
	  egdim[i] = ME_GEntDim(redges[i]);
	  egid[i] = ME_GEntID(redges[i]);
	}
      }


      for (i = 0; i < nf; i++) {
	fgdim = 4;
	fgid = 0;

	nfe = MSTK_rfe_template[r->mrtype][i][0];
	for (j = 0; j < nfe; j++) {
	  ind = MSTK_rfe_template[r->mrtype][i][j+1];
	  sgn = MSTK_rfedir_template[r->mrtype][i][j+1];
	  fedges[j] = redges[ind];
	  fedirs[j] = !(redirs[ind]^sgn);

	  if (fgdim == 4 || egdim[ind] > fgdim) {
	    fgdim = egdim[ind];
	    fgid = egid[ind];
	  }
	}

	rfaces[i] = MEs_CommonFace(nfe, fedges);

	if (!rfaces[i]) {
	  rfaces[i] = MF_New(mesh);
	  rfdirs[i] = MSTK_rfdir_template[r->mrtype][i];

	  MF_Set_Edges(rfaces[i], nfe, fedges, fedirs);

	  if (fgdim >= 2) {
	    MF_Set_GEntDim(rfaces[i],fgdim);
	    MF_Set_GEntID(rfaces[i],fgid);
	  }
	  else {
	    /* Cannot determine unique classification for face */
	    MSTK_Report("MR_Set_Vertices",
			"Cannot determine face classification. Guessing...",
			MSTK_WARN);
	    MF_Set_GEntDim(rfaces[i],(fgdim+1));
	    MF_Set_GEntID(rfaces[i],0);
	  }


	}
	else {
	  fregs = MF_Regions(rfaces[i]);
	  if (!fregs) {
	    ind = MSTK_rfv_template[r->mrtype][i][1];
	    verts[0] = mvertices[ind];
	    ind = MSTK_rfv_template[r->mrtype][i][2];
	    verts[1] = mvertices[ind];

	    fverts = MF_Vertices(rfaces[i],1,verts[0]);
	    rfdirs[i] = ((verts[1] == List_Entry(fverts,1)) ? 
			 MSTK_rfdir_template[r->mrtype][i] : 
			 !MSTK_rfdir_template[r->mrtype][i]);
	    List_Delete(fverts);
	  }
	  else {
#ifdef DEBUG
	    if (List_Num_Entries(fregs) > 1)
	      MSTK_Report("MR_Set_Vertices",
			  "Face already connected on both sides",MSTK_FATAL);
#endif
	    oregion = List_Entry(fregs,0);
	    odir = MR_FaceDir(oregion,rfaces[i]);
	    rfdirs[i] = !odir;

	    List_Delete(fregs);
	  }
	}

      }

      MR_Set_Faces(r, nf, rfaces, rfdirs);
    }
  }

  int MR_Num_Faces_FNR3R4(MRegion_ptr r) {
    List_ptr rfaces = ((MRegion_Adj_FN *)r->adj)->rfaces;
    return List_Num_Entries(rfaces);
  }


  List_ptr MR_Faces_FNR3R4(MRegion_ptr r) {
    MRegion_Adj_FN *adj;
    adj = (MRegion_Adj_FN *) r->adj;
    return List_Copy(adj->rfaces);
  }

  List_ptr MR_AdjRegions_FNR3R4(MRegion_ptr r) {
    int i, nf;
    MRegion_ptr freg;
    MFace_ptr face;
    List_ptr adjr;
    MRegion_Adj_FN *adj;

    adj = (MRegion_Adj_FN *) r->adj;
    nf = List_Num_Entries(adj->rfaces);

    adjr = List_New(4);
    for (i = 0; i < nf; i++) {
      face = List_Entry(adj->rfaces,i);
      freg = MF_Region(face,0);
      if (freg) {
	if (freg == r) {
	  freg = MF_Region(face,1);
	  if (freg) {
   	    List_Add(adjr,freg);
  	  }
	}
	else 
	  List_Add(adjr,freg);
      }
    }
    return adjr;
  }

  int MR_Rev_FaceDir_FNR3R4(MRegion_ptr r, MFace_ptr f) {
    int i,j,k, nf;
    MRegion_Adj_FN *adj;

    adj = (MRegion_Adj_FN *) r->adj;
    nf = List_Num_Entries(adj->rfaces);

    for (i = 0; i < nf; i++) {

      if (f == (MFace_ptr) List_Entry(adj->rfaces,i)) {

	j = (int) i/(8*sizeof(unsigned int));
	k = i%(8*sizeof(unsigned int));

	adj->fdirs[j] ^= (1UL << k);
	return 1;
      }

    }

    return -1;
  }

  int MR_Rev_FaceDir_i_FNR3R4(MRegion_ptr r, int i) {
    int j, k;
    MRegion_Adj_FN *adj;
    adj = (MRegion_Adj_FN *) r->adj;

    if (i >= MAXPF3)
      MSTK_Report("MR_FaceDir_i","Not that many faces in region",MSTK_ERROR);
    
    j = (int) i/(8*sizeof(unsigned int));
    k = i%(8*sizeof(unsigned int));

    adj->fdirs[j] ^= (1UL << k);
    return 1;
  }

  int MR_FaceDir_FNR3R4(MRegion_ptr r, MFace_ptr f) {
    int i,j,k, nf;
    MRegion_Adj_FN *adj;

    adj = (MRegion_Adj_FN *) r->adj;
    nf = List_Num_Entries(adj->rfaces);

    for (i = 0; i < nf; i++) {

      if (f == (MFace_ptr) List_Entry(adj->rfaces,i)) {

	j = (int) i/(8*sizeof(unsigned int));
	k = i%(8*sizeof(unsigned int));

	return ((adj->fdirs[j])>>k & 1);
      }

    }

    return -1;
  }

  int MR_FaceDir_i_FNR3R4(MRegion_ptr r, int i) {
    int j, k;
    MRegion_Adj_FN *adj;
    adj = (MRegion_Adj_FN *) r->adj;

    if (i >= MAXPF3)
      MSTK_Report("MR_FaceDir_i","Not that many faces in region",MSTK_ERROR);
    
    j = (int) i/(8*sizeof(unsigned int));
    k = i%(8*sizeof(unsigned int));

    return ((adj->fdirs[j])>>k & 1);
  }






  void MR_Rem_Face_FNR3R4(MRegion_ptr r, MFace_ptr remface) {
    int i, j, k, j1, k1, nf, findex, dir, found;
    MRegion_Adj_FN *adj;

    adj = (MRegion_Adj_FN *) r->adj;
    nf = List_Num_Entries(adj->rfaces);

    if (nf == 0)
      MSTK_Report("MR_Remove_Faces_FNR3R4","No initial set of faces for region",
		  MSTK_ERROR);

    for (j = 0, found = 0; j < nf; j++)
      if (List_Entry(adj->rfaces,j) == remface) {
	found = 1;
	findex = j;
	break;
      }
    if (!found) {
      MSTK_Report("MR_Rem_Face_FNR3R4","Face not found in region",MSTK_ERROR);
      return;
    }


    List_Remi(adj->rfaces,findex);

    /* Move the remaining bits to the right */
    
    for (i = findex; i < nf-1; i++) {

      j = (int) i/(8*sizeof(unsigned int));
      k = i%(8*sizeof(unsigned int));
      
      adj->fdirs[j] = (adj->fdirs[j] & ~(1<<k)); /* set bit i to 0 */
      
      /* get bit for i+1 */
      j1 = (int) (i+1)/(8*sizeof(unsigned int));
      k1 = (i+1)%(8*sizeof(unsigned int));
      
      dir = (adj->fdirs[j1]>>k1) & 1;
      
      /* move bit for i+1 to i */
      
      adj->fdirs[j] = (adj->fdirs[j] | (dir<<k)); 
      
    }

    nf--;

      
    /* Tell face that it is no longer bounding this region */

    MF_Rem_Region(remface,r);

  }





  /* If we allow replacing a face with multiple faces, we have to check if 
     we need to expand adj->fdirs */

  void MR_Replace_Face_FNR3R4(MRegion_ptr r, MFace_ptr f, MFace_ptr nuf, int nudir) {
    int i, j, k, nf;
    MRegion_Adj_FN *adj;

#ifdef DEBUG
    if (MR_Mesh(r) != MF_Mesh(nuf))
      MSTK_Report("MR_Replace_Face_FNR3R4",
		  "Region and Face belong to different meshes",MSTK_FATAL);
#endif

    adj = (MRegion_Adj_FN *) r->adj;
    nf = List_Num_Entries(adj->rfaces);

    for (i = 0; i < nf; i++)
      if (f == (MFace_ptr) List_Entry(adj->rfaces,i)) {

	j = (int) i/(8*sizeof(unsigned int));
	k = i%(8*sizeof(unsigned int));

	adj->fdirs[j] = (adj->fdirs[j] & ~(1<<k)); /* set bit i to 0 */
	adj->fdirs[j] = (adj->fdirs[j] | (nudir<<k)); /* set to nudir*/

	List_Replacei(adj->rfaces,i,nuf);

	MF_Rem_Region(f,r);
	MF_Add_Region(nuf,r,!nudir);
	return;
      }
  }

  void MR_Replace_Face_i_FNR3R4(MRegion_ptr r, int i, MFace_ptr nuf, int nudir) {
    int j, k;
    MRegion_Adj_FN *adj;
    MFace_ptr f;

    if (i >= MAXPF3)
      MSTK_Report("MR_FaceDir_i","Not that many faces in region",MSTK_ERROR);

#ifdef DEBUG
    if (MR_Mesh(r) != MF_Mesh(nuf))
      MSTK_Report("MR_Replace_Face_FNR3R4",
		  "Region and Face belong to different meshes",MSTK_FATAL);
#endif

    adj = (MRegion_Adj_FN *) r->adj;
    f = List_Entry(adj->rfaces,i);

    j = (int) i/(8*sizeof(unsigned int));
    k = i%(8*sizeof(unsigned int));
    adj->fdirs[j] = (adj->fdirs[j] & ~(1<<k)); /* set bit i to 0 */
    adj->fdirs[j] = (adj->fdirs[j] | (nudir<<k)); /* set to nudir*/

    List_Replacei(adj->rfaces,i,nuf);

    MF_Rem_Region(f,r);
    MF_Add_Region(nuf,r,!nudir);
  }

  int MR_UsesFace_FNR3R4(MRegion_ptr r, MFace_ptr f) {
    MRegion_Adj_FN *adj;

    adj = (MRegion_Adj_FN *) r->adj;
    return List_Contains(adj->rfaces,f);
  }


#ifdef __cplusplus
}
#endif
