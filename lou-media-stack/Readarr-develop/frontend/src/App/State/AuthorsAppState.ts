import AppSectionState, {
  AppSectionDeleteState,
  AppSectionSaveState,
} from 'App/State/AppSectionState';
import Author from 'Author/Author';

interface AuthorsAppState
  extends AppSectionState<Author>,
    AppSectionDeleteState,
    AppSectionSaveState {
  itemMap: Record<number, number>;

  deleteOptions: {
    addImportListExclusion: boolean;
  };
}

export default AuthorsAppState;
