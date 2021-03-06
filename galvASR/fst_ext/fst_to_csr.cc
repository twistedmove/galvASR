namespace galvASR { namespace fst_ext {

template<typename Fst>
void fstToCSRMemoryRequirements(const Fst& fst, size_t *num_vertices,
                                size_t *num_edges) {
  *num_vertices = fst.NumStates();
  *num_edges = fst.NumArcs();
}

template<typename Fst>
void fstToCSR(const Fst& fst,  const size_t num_vertices,
              const size_t num_edges, size_t *row_offsets,
              size_t *column_indices, Fst::Weight *nonzero_values) {

  size_t next_non_zero_index = 0;
  row_offsets[0] = 0;
  StateId previous_state_id = -1;
  for (fst::StateIterator<Fst> s_iter(fst); !s_iter.Done(); s_iter.Next()) {
    StateId state_id = s_iter.Value();
    assert(prevous_state_id + 1 == state_id);
    // WARNING: state ids must be contiguously increasing!!!

    for (fst::ArcIterator<Fst> a_iter(fst, state_id); !a_iter.Done();
         a_iter.Next()) {
      const Fst::Arc& arc = a_iter.Value();
      column_indices[next_non_zero_index] = arc.nextstate;
      data[next_non_zero_index] = arc.weight;
      ++next_non_zero_index;
    }
    row_offsets[state_id + 1] = next_non_zero_index;
  }
}


// Hopefully this operation can be done with only the C API or the public C++ API.
struct TensorflowCSR {
  TF_Tensor nonzero_values;
  TF_Tensor row_offsets;
  TF_Tensor column_indices;
  size_t nnz;
  union {
    size_t num_rows;
    size_t num_columns;
  };

  ~TensorflowCSR() {
    // Do we really want this?
    TF_DeleteTensor(nonzero_values);
    TF_DeleteTensor(row_offsets);
    TF_DeleteTensor(column_indices);
  }
};

// Probably want a macro here to compile only if building with Tensorflow
template<typename Fst>
TensorflowCSR fstToSparseTFTensor(const Fst& fst) {
  TensorflowCSR csr{};

  fstToCSRMemoryRequirements(fst, &csr.num_rows, &nnz);
  csr.nonzero_values = TF_AllocateTensor(TF_FLOAT, {csr.nnz}, 1, csr.nnz);
  csr.row_offsets = TF_AllocateTensor(TF_UINT32, {csr.num_rows + 1}, 1, csr.num_rows + 1);
  csr.column_indices = TF_AllocateTensor(TF_UINT32, {csr.nnz}, 1, csr.nnz);
  Fst::Weight *values = (Fst::Weight*) TF_TensorData(&csr.nonzero_values);
  size_t *row_offsets = (size_t*) TF_TensorData(&csr.row_offsets);
  size_t *column_indices = (size_t*) TF_TensorData(&csr.column_indices);

  fstToCSR(fst, csr.num_rows, csr.nnz, row_offsets, column_indices,
           Fst::Weight *data);
  return csr;
}

} // namespace galvASR
} // namespace fst_ext
