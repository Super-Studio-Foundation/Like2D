document.addEventListener('DOMContentLoaded', function() {
    const navLinks = document.querySelectorAll('.nav-link');
    const sections = document.querySelectorAll('section[id]');

    function setActiveLink() {
        let current = '';
        sections.forEach(section => {
            const sectionTop = section.offsetTop - 100;
            if (window.scrollY >= sectionTop) {
                current = section.getAttribute('id');
            }
        });

        navLinks.forEach(link => {
            link.classList.remove('active');
            if (link.getAttribute('href') === '#' + current) {
                link.classList.add('active');
            }
        });
    }

    window.addEventListener('scroll', setActiveLink);

    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            const href = this.getAttribute('href');
            if (href.startsWith('#')) {
                e.preventDefault();
                const target = document.querySelector(href);
                if (target) {
                    target.scrollIntoView({
                        behavior: 'smooth',
                        block: 'start'
                    });
                }
            }
        });
    });

    // --- Copy Code functionality ---
    document.querySelectorAll('.doc-pre').forEach(block => {
        // Create the button
        const button = document.createElement('button');
        button.className = 'copy-btn';
        button.innerText = 'Copy';
        
        button.addEventListener('click', () => {
            // Find the code element and get its text
            const codeEl = block.querySelector('code');
            if (!codeEl) return;
            
            const code = codeEl.innerText;
            
            navigator.clipboard.writeText(code).then(() => {
                button.innerText = 'Copied!';
                button.classList.add('copied');
                
                setTimeout(() => {
                    button.innerText = 'Copy';
                    button.classList.remove('copied');
                }, 2000);
            }).catch(err => {
                console.error('Failed to copy: ', err);
                button.innerText = 'Error';
            });
        });
        
        block.appendChild(button);
    });
});
