# Contributing to Like2D Framework

Thank you for your interest in contributing to the Like2D Framework! This document provides guidelines and requirements for contributing to this SSOFoundation project.

## Project Overview

Like2D is a lightweight, high-performance 2D game framework built with SDL3 and Luau scripting language. It's designed to be modular, efficient, and easy to use for 2D game development.

## Contribution Requirements

### SSOFoundation Project
Like2D is an official SSOFoundation project. All contributions must be made via **Pull Requests** and will be reviewed according to SSOFoundation standards.

### Pull Request Process
1. **Fork** the repository
2. **Create** a feature branch from `main`
3. **Implement** your changes following the guidelines below
4. **Test** thoroughly using the build system
5. **Submit** a Pull Request with detailed description
6. **Wait** for review and approval

## Technical Requirements

### C++20 Standard
All code must be written in **C++20** and follow modern C++ practices:
- Use `auto` where appropriate
- Prefer range-based for loops
- Use smart pointers over raw pointers
- Follow RAII principles
- Use `constexpr` and `const` correctly

### Modular Structure
Like2D follows a strict modular architecture:

```
src/
├── core/           # Engine core systems
├── renderer/       # Rendering subsystem
├── scripting/      # Luau bindings
└── Like2D.cpp     # Main application class
```

**Guidelines:**
- Keep modules independent and loosely coupled
- Use header guards consistently
- Follow the existing naming conventions
- Maintain clear separation of concerns

### Code Style
- **Functions**: `camelCase()` for functions
- **Variables**: `camelCase` for variables
- **Classes**: `PascalCase` for classes
- **Constants**: `UPPER_SNAKE_CASE` for constants
- **Files**: `PascalCase.cpp/.h` for source files

## Testing Requirements

### Mandatory Testing
All new code **must** be tested before submission:

#### Build Testing
```bash
# Test incremental build
build.bat

# Test full build (if dependencies changed)
build-full.bat

# Clean build test
build.bat clean
build.bat
```

#### Runtime Testing
- Test with `Like2D.exe` (release mode)
- Test with `LikeC.exe` (debug mode)
- Verify all new features work correctly
- Check for memory leaks or crashes

### Test Coverage
- New functions must have basic testing
- API changes require comprehensive testing
- Build system changes require both build methods testing
- Documentation updates should be verified

## Contribution Types

### Bug Fixes
- Clearly describe the bug being fixed
- Include steps to reproduce the issue
- Add tests to prevent regression
- Update documentation if necessary

### New Features
- Design features following the modular structure
- Include comprehensive documentation
- Add examples or tutorials if appropriate
- Consider backward compatibility

### Documentation
- Follow existing documentation style
- Include code examples
- Update table of contents if needed
- Test all code examples

### Build System
- Test both `build.bat` and `build-full.bat`
- Ensure Windows compatibility
- Document any new requirements
- Test clean builds

## Submission Guidelines

### Pull Request Requirements
Each Pull Request must include:

1. **Clear Title**: Descriptive of the changes
2. **Detailed Description**: What and why changes were made
3. **Testing Evidence**: Confirmation of successful testing
4. **Documentation Updates**: If applicable
5. **Breaking Changes**: Clearly noted if any

### Pull Request Template
```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation
- [ ] Build system
- [ ] Other

## Testing
- [ ] Tested with build.bat
- [ ] Tested with build-full.bat
- [ ] Tested Like2D.exe
- [ ] Tested LikeC.exe
- [ ] No regressions detected

## Checklist
- [ ] Code follows C++20 standards
- [ ] Modular structure maintained
- [ ] Documentation updated
- [ ] Build system works
- [ ] No breaking changes (or clearly documented)
```

## Review Process

### Approval Authority
The **final approval** for all contributions resides with the **Lead Developer (Rozak)**. All Pull Requests must be approved by Rozak before merging.

### Review Criteria
- Code quality and C++20 compliance
- Modular architecture adherence
- Build system compatibility
- Documentation completeness
- Testing thoroughness
- Project alignment

### Review Timeline
- Initial review: 1-3 business days
- Additional rounds: As needed
- Final approval: By Lead Developer (Rozak)

## Development Guidelines

### Before Starting
1. Read existing code to understand patterns
2. Check for similar existing implementations
3. Plan your approach following modular design
4. Consider impact on existing functionality

### During Development
1. Write clean, readable code
2. Add appropriate comments
3. Follow existing error handling patterns
4. Test incrementally as you develop

### Before Submission
1. Run full build tests
2. Test all affected functionality
3. Update documentation
4. Clean up any debug code
5. Ensure no warnings in build

## Communication

### Getting Help
- Review existing documentation
- Check issues for similar problems
- Ask questions in Pull Request discussions
- Respect the review process

### Reporting Issues
- Use GitHub Issues with clear descriptions
- Include steps to reproduce
- Provide system information
- Attach relevant logs if available

## Community Standards

### Code of Conduct
- Be respectful and professional
- Provide constructive feedback
- Welcome new contributors
- Focus on what is best for the project

### Collaboration
- Work openly and transparently
- Share knowledge and help others
- Accept feedback gracefully
- Credit contributors appropriately

## Recognition

### Contributor Credit
- Contributors are recognized in updates.txt
- Significant contributions noted in README.md
- Special contributors mentioned in release notes

### SSOFoundation Recognition
- Outstanding contributors may receive SSOFoundation recognition
- Long-term contributors may be offered maintainer roles
- Valuable contributions may be featured in project showcases

## Legal and Licensing

### Contribution Agreement
By contributing to Like2D, you agree that:
- Your contribution may be used under the project license
- You have the right to make the contribution
- Your contribution follows all applicable laws

### Intellectual Property
- Respect existing licenses and copyrights
- Do not include proprietary code
- Document any third-party dependencies
- Follow open source best practices

## Frequently Asked Questions

### Q: Can I contribute without following C++20?
A: No, C++20 is mandatory for all new code to maintain consistency and performance.

### Q: Do I need to test both build methods?
A: Yes, both `build.bat` and `build-full.bat` must be tested for any changes.

### Q: How long does review take?
A: Initial review typically takes 1-3 business days, but complex changes may take longer.

### Q: Can I submit breaking changes?
A: Breaking changes are allowed but must be clearly documented and justified.

### Q: Who has final approval?
A: The Lead Developer (Rozak) has final approval authority for all contributions.

## Contact Information

### Project Lead
- **Lead Developer**: Rozak
- **Organization**: SSOFoundation
- **Project**: Like2D Framework

### Additional Resources
- [Documentation](docs/)
- [API Reference](docs/API_REFERENCE.md)
- [Tutorials](docs/TUTORIALS.md)
- [Build System](docs/BUILD_SYSTEM.md)

---

Thank you for contributing to Like2D! Your contributions help make this framework better for everyone.

**SSOFoundation - Building the Future of Game Development**
