import classNames from 'classnames';
import PropTypes from 'prop-types';
import React from 'react';
import formatBytes from 'Utilities/Number/formatBytes';
import EnhancedSelectInputOption from './EnhancedSelectInputOption';
import styles from './RootFolderSelectInputOption.css';

function RootFolderSelectInputOption(props) {
  const {
    id,
    value,
    name,
    freeSpace,
    authorFolder,
    isMissing,
    isMobile,
    isWindows,
    ...otherProps
  } = props;

  const slashCharacter = isWindows ? '\\' : '/';

  const text = name === '' ? value : `[${name}] ${value}`;

  return (
    <EnhancedSelectInputOption
      id={id}
      isMobile={isMobile}
      {...otherProps}
    >
      <div className={classNames(
        styles.optionText,
        isMobile && styles.isMobile
      )}
      >
        <div className={styles.value}>
          {text}

          {
            authorFolder && id !== 'addNew' ?
              <div className={styles.authorFolder}>
                {slashCharacter}
                {authorFolder}
              </div> :
              null
          }
        </div>

        {
          freeSpace == null ?
            null :
            <div className={styles.freeSpace}>
              {formatBytes(freeSpace)} Free
            </div>
        }

        {
          isMissing ?
            <div className={styles.isMissing}>
              Missing
            </div> :
            null
        }
      </div>
    </EnhancedSelectInputOption>
  );
}

RootFolderSelectInputOption.propTypes = {
  id: PropTypes.string.isRequired,
  name: PropTypes.string.isRequired,
  value: PropTypes.string.isRequired,
  freeSpace: PropTypes.number,
  authorFolder: PropTypes.string,
  isMissing: PropTypes.bool,
  isMobile: PropTypes.bool.isRequired,
  isWindows: PropTypes.bool
};

RootFolderSelectInputOption.defaultProps = {
  name: ''
};

export default RootFolderSelectInputOption;
